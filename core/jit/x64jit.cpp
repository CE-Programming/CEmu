#if defined(JIT_SUPPORT) && defined(JIT_BACKEND_X64)

#include "jit.h"

#include "../cpu.h"
#include "../flash.h"
#include "../mem.h"

#include <asmjit/asmjit.h>
using namespace asmjit;

#include "Judy++.h"
using namespace judy;

#include <iostream>
#include <type_traits>
#include <utility>

namespace asmjit {
namespace x86 {

enum class Flag : uint8_t {
    Carry    =  0,
    Parity   =  2,
    AuxCarry =  4,
    Zero     =  6,
    Sign     =  7,
    Overflow = 11,
    End,
};

static Cond::Value condFromFlag(Flag flag) noexcept {
    switch (flag) {
        case Flag::Carry:    return Cond::kC;
        case Flag::Parity:   return Cond::kP;
        case Flag::Zero:     return Cond::kZ;
        case Flag::Sign:     return Cond::kS;
        case Flag::Overflow: return Cond::kO;
        default: assert(0);
    }
}

static Inst::Id setccFromFlag(Flag flag) noexcept {
    switch (flag) {
        case Flag::Carry:    return Inst::kIdSetc;
        case Flag::Parity:   return Inst::kIdSetp;
        case Flag::Zero:     return Inst::kIdSetz;
        case Flag::Sign:     return Inst::kIdSets;
        case Flag::Overflow: return Inst::kIdSeto;
        default: assert(0);
    }
}

static constexpr Gp gpb(Gp::Id rId, bool hi) noexcept {
    return Gp(hi ? GpbHi::kSignature : GpbLo::kSignature, rId);
}

static constexpr bool hasHi(Gp::Id rId) noexcept {
    return rId < RegTraits<Reg::kTypeGpbHi>::kCount;
}

}
}

namespace {

template<typename Array>
constexpr std::size_t lengthof(const Array &) { return std::extent<Array>::value; }

template<typename Enum>
constexpr typename std::underlying_type<Enum>::type u(Enum value) {
    return typename std::underlying_type<Enum>::type(value);
}

struct CodeBlock {
    uint32_t (*entry)(eZ80cpu_t *, int32_t);
    uint32_t start, end, size, cycles;
};
extern "C" {
[[noreturn]] void jitBind();
void jitPatch(void *target);
}

uint64_t blocksExecuted, unhandledValues[256];

JitRuntime runtime;
uint32_t (*dispatch)(eZ80cpu_t *, int32_t, uint32_t (*)(eZ80cpu_t *, int32_t));
Judy<uint32_t, bool> fetched;
Judy<uint32_t, CodeBlock *> blocks;
Judy<void *, uint32_t> patchPoints;
Zone blockZone(0x1000 - Zone::kBlockOverhead, alignof(CodeBlock));
ZoneAllocator blockAlloc;

void init(CodeHolder &code, BaseEmitter &e) {
    code.init(runtime.codeInfo());
    code.addEmitterOptions(BaseAssembler::kOptionStrictValidation |
                           BaseAssembler::kOptionOptimizedForSize |
                           BaseAssembler::kOptionOptimizedAlign);
    code.attach(&e);
}

enum AddressFlag : uint32_t {
      kAdlFlag   = Support::bitMask(24),
      kEventFlag = Support::bitMask(25),
};

namespace z80 {

enum class Reg : uint8_t {
    I,
    MBR,
    SPS,
    AF,
    BC,
    DE,
    HL,
    IX,
    IY,
    SPL,
    TEMP,
};
enum class Flag : uint8_t {
    Carry,
    AddSub,
    ParityOverflow,
    X,
    HalfCarry,
    Y,
    Zero,
    Sign,
    End,
};

struct RegState {
    uint8_t x86Reg = Support::allOnes<typeof(x86Reg)>();
    uint8_t rotAmt = 0;
    bool dirty = false;
};
inline uint8_t regSize(Reg reg) {
    return (1 + (u(reg) > u(Reg::AF))) << 1;
}
}

class FlagState {
public:
    enum class Kind : uint8_t {
        Unknown,
        InZ80Flag,
        InX86Flag,
        Constant,
    };
    FlagState() : state(Kind::Unknown) {}
    FlagState(z80::Flag flag) : state(Kind::InZ80Flag), z80(flag) {}
    FlagState(x86::Flag flag) : state(Kind::InX86Flag), x86(flag) {}
    FlagState(bool value) : state(Kind::Constant), constant(value) {}
    Kind kind() const { return state; }
    bool isInZ80Flag () const { return state == Kind::InZ80Flag; }
    bool isInX86Flag () const { return state == Kind::InX86Flag; }
    bool isConstant() const { return state == Kind::Constant; }
    z80::Flag z80Flag()  const { assert(isInZ80Flag()); return z80; }
    x86::Flag x86Flag()  const { assert(isInX86Flag()); return x86; }
    bool constantValue() const { assert(isConstant()); return constant; }
private:
    Kind state;
    union {
        z80::Flag z80;
        x86::Flag x86;
        bool constant;
    };
};

const std::uint16_t calleeSave = Support::bitMask(x86::Gp::kIdBx,
                                                  x86::Gp::kIdBp,
                                                  x86::Gp::kIdR12,
                                                  x86::Gp::kIdR13,
                                                  x86::Gp::kIdR14,
                                                  x86::Gp::kIdR15);
struct Gen {
    CodeBlock &block;
    CodeHolder code;
    x86::Assembler a;
    Label eventLabel;
    FlagState z80Flags[8] = {
        z80::Flag::Carry,
        z80::Flag::AddSub,
        z80::Flag::ParityOverflow,
        z80::Flag::X,
        z80::Flag::HalfCarry,
        z80::Flag::Y,
        z80::Flag::Zero,
        z80::Flag::Sign,
    }, x86Flags[12] = {
        x86::Flag::Carry,
        true,
        x86::Flag::Parity,
        false,
        x86::Flag::AuxCarry,
        false,
        x86::Flag::Zero,
        x86::Flag::Sign,
        {}, {}, {},
        x86::Flag::Overflow,
    };
    z80::RegState z80Regs[11];
    std::uint16_t x86RegsInUse = Support::bitMask(x86::Gp::kIdSp,   // reserved for stack
                                                  x86::Gp::kIdDi,   // reserved for cpu
                                                  x86::Gp::kIdSi) | // reserved for cycles
        calleeSave;
    std::int32_t pc, cycles = 0;
    std::size_t cycleOffset;
    Gen(CodeBlock &block, std::uint32_t pc) : block(block), pc(pc) {
        block.start = pc;
        init(code, a);
        eventLabel = a.newLabel();
    }

    x86::Mem reg_ptr(z80::Reg reg) {
        static const std::size_t offsets[] = {
            offsetof(eZ80cpu_t, registers.I),
            offsetof(eZ80cpu_t, registers.R),
            offsetof(eZ80cpu_t, registers.SPS),
            offsetof(eZ80cpu_t, registers.AF),
            offsetof(eZ80cpu_t, registers.BC),
            offsetof(eZ80cpu_t, registers.DE),
            offsetof(eZ80cpu_t, registers.HL),
            offsetof(eZ80cpu_t, registers.IX),
            offsetof(eZ80cpu_t, registers.IY),
            offsetof(eZ80cpu_t, registers.SPL),
            {},
        };
        return a.ptr_zdi(offsets[u(reg)], regSize(reg));
    }

    static x86::Flag next(x86::Flag flag) { return x86::Flag(u(flag) + 1); }
    static z80::Flag next(z80::Flag flag) { return z80::Flag(u(flag) + 1); }
    static z80::Reg  next(z80::Reg  reg ) { return z80::Reg (u(reg ) + 1); }

    FlagState &state(z80::Flag z80Flag) {
        auto index = u(z80Flag);
        assert(index < lengthof(z80Flags));
        return z80Flags[index];
    }
    FlagState &state(x86::Flag x86Flag) {
        auto index = u(x86Flag);
        assert(index < lengthof(x86Flags));
        return x86Flags[index];
    }
    z80::RegState &state(z80::Reg z80Reg) {
        auto index = u(z80Reg);
        assert(index < lengthof(z80Regs));
        return z80Regs[index];
    }

    void prolog() {
        cycleOffset = a.offset();
        a.short_().sub(x86::esi, 0);
        a.notTaken().jg(eventLabel);
    }
    void fixup() {
        std::size_t offset = a.offset();
        a.setOffset(cycleOffset);
        a.short_().sub(x86::esi, -block.cycles);
        a.setOffset(offset);
    }

    x86::Gp::Id alloc(z80::Reg z80Reg) {
        if (state(z80Reg).x86Reg == Support::allOnes<typeof(std::declval<z80::RegState>().x86Reg)>()) {
            if (z80Reg == z80::Reg::AF) {
                state(z80Reg).x86Reg = x86::Gp::kIdBx;
            } else {
                state(z80Reg).x86Reg = Support::ctz(~x86RegsInUse);
                x86RegsInUse |= x86RegsInUse + 1;
            }
        }
        return x86::Gp::Id(state(z80Reg).x86Reg);
    }

    x86::Gp::Id mat(z80::Reg z80Reg, bool dirty = false) {
        state(z80Reg).dirty |= dirty;
        if (state(z80Reg).x86Reg == Support::allOnes<typeof(std::declval<z80::RegState>().x86Reg)>()) {
            x86::Inst::Id inst = x86::Inst::kIdMovzx;
            inst = regSize(z80Reg) == 2 ? inst = x86::Inst::kIdMovzx : x86::Inst::kIdMov;
            a.emit(inst, x86::gpd(alloc(z80Reg)), reg_ptr(z80Reg));
        }
        return x86::Gp::Id(state(z80Reg).x86Reg);
    }

    Error flush(z80::Reg z80Reg) {
        if (state(z80Reg).dirty) {
            ASMJIT_PROPAGATE(setRotAmt(z80Reg));
            ASMJIT_PROPAGATE(a.mov(reg_ptr(z80Reg), x86::gpd(state(z80Reg).x86Reg)));
            state(z80Reg).x86Reg = Support::allOnes<typeof(std::declval<z80::RegState>().x86Reg)>();
            state(z80Reg).dirty = false;
        }
        return kErrorOk;
    }

    void flush() {
        for (z80::Reg z80Reg = next(z80::Reg::AF); z80Reg != z80::Reg::TEMP; z80Reg = next(z80Reg))
            flush(z80Reg);
        uint8_t constOr = 0, constAnd = Support::allOnes<typeof(constAnd)>(),
            numX86Flags = 0, matchingFlags = 0;
        uint16_t x86Flags = 0;
        for (z80::Flag z80Flag{}; z80Flag != z80::Flag::End; z80Flag = next(z80Flag)) {
            switch (state(z80Flag).kind()) {
                case FlagState::Kind::Unknown:
                    assert(0);
                case FlagState::Kind::InZ80Flag:
                    assert(state(z80Flag).z80Flag() == z80Flag);
                    break;
                case FlagState::Kind::InX86Flag:
                    constAnd &= ~Support::bitMask(u(z80Flag));
                    numX86Flags += !Support::bitTest(x86Flags, u(state(z80Flag).x86Flag()));
                    if (u(state(z80Flag).x86Flag()) == u(z80Flag))
                        matchingFlags |= Support::bitMask(u(z80Flag));
                    x86Flags |= Support::bitMask(u(state(z80Flag).x86Flag()));
                    break;
                case FlagState::Kind::Constant:
                    if (state(z80Flag).constantValue())
                        constOr |= Support::bitMask(u(z80Flag));
                    else
                        constAnd &= ~Support::bitMask(u(z80Flag));
                    break;
            }
        }
        assert(numX86Flags == Support::popcnt(x86Flags));
        if (numX86Flags <= 2 && !Support::bitTest(x86Flags, u(x86::Flag::AuxCarry))) {
            bool high = false;
            x86::Flag x86Flag[2];
            uint8_t rotAmts[2] = { 0, 0 };
            for (Support::BitWordIterator<typeof(x86Flags)> it(x86Flags); it.hasNext(); high = true)
                a.emit(x86::setccFromFlag(x86Flag[high] = x86::Flag(it.next())),
                       x86::gpb(x86::Gp::kIdAx, high));
            if (constAnd != Support::allOnes<typeof(constAnd)>())
                a.and_(x86::gpb_lo(mat(z80::Reg::AF, true)), constAnd);
            if (constOr != 0)
                a.or_(x86::gpb_lo(mat(z80::Reg::AF, true)), constOr);
            for (z80::Flag z80Flag{}; z80Flag != z80::Flag::End; z80Flag = next(z80Flag)) {
                if (!state(z80Flag).isInX86Flag())
                    continue;
                high = state(z80Flag).x86Flag() != x86Flag[0];
                auto x86Reg = x86::gpb(x86::Gp::kIdAx, high);
                if (rotAmts[high] != u(z80Flag)) {
                    a.rol(x86Reg, (u(z80Flag) - rotAmts[high]) & 7);
                    rotAmts[high] = u(z80Flag);
                }
                a.or_(x86::gpb_lo(mat(z80::Reg::AF, true)), x86Reg);
            }
        } else {
            bool haveLahf = CpuInfo::host().archId() == ArchInfo::kIdX86 ||
                            CpuInfo::host().features<x86::Features>().hasLAHFSAHF();
            uint8_t overflowRotAmt;
            x86::Gp x86Reg = x86::gpb(x86::Gp::kIdAx, haveLahf),
                overflowX86Reg = x86::gpb(x86::Gp::kIdAx, !haveLahf);
            if (haveLahf) {
                a.lahf();
                overflowRotAmt = 0;
                if (Support::bitTest(x86Flags, u(x86::Flag::Overflow)))
                    a.seto(overflowX86Reg);
            } else {
                a.pushfd(); // pushfq in 64-bit mode
                a.pop(a.zax());
                overflowRotAmt = u(x86::Flag::Overflow) - 8;
                if (Support::bitTest(x86Flags, u(x86::Flag::Overflow)))
                    a.and_(overflowX86Reg, Support::bitMask(overflowRotAmt));
            }
            // Try to expand matchingFlags
            if (matchingFlags) {
                for (x86::Flag x86Flag{}; x86Flag != x86::Flag::End; x86Flag = next(x86Flag)) {
                    if (!state(x86Flag).isConstant())
                        continue;
                    if (!state(x86Flag).constantValue()) {
                        matchingFlags |= Support::bitMask(u(x86Flag));
                    } else if (Support::bitTest(constOr, u(x86Flag))) {
                        constOr &= ~Support::bitMask(u(x86Flag));
                        matchingFlags |= Support::bitMask(u(x86Flag));
                    }
                }
            }
            if (constAnd != Support::allOnes<typeof(constAnd)>())
                a.and_(x86::gpb_lo(mat(z80::Reg::AF, true)), constAnd);
            if (constOr != 0)
                a.or_(x86::gpb_lo(mat(z80::Reg::AF, true)), constOr);
            if (matchingFlags) {
                a.and_(x86Reg, matchingFlags);
                a.or_(x86::gpb_lo(mat(z80::Reg::AF, true)), x86Reg);
            }
            for (z80::Flag z80Flag{}; z80Flag != z80::Flag::End; z80Flag = next(z80Flag)) {
                if (!state(z80Flag).isInX86Flag() ||
                    state(z80Flag).x86Flag() != x86::Flag::Overflow)
                    continue;
                if (overflowRotAmt != u(z80Flag)) {
                    a.rol(overflowX86Reg, (u(z80Flag) - overflowRotAmt) & 7);
                    overflowRotAmt = u(z80Flag);
                }
                a.or_(x86::gpb_lo(mat(z80::Reg::AF, true)), overflowX86Reg);
            }
            for (z80::Flag z80Flag{}; z80Flag != z80::Flag::End; z80Flag = next(z80Flag)) {
                if (!state(z80Flag).isInX86Flag() ||
                    Support::bitTest(matchingFlags, u(state(z80Flag).x86Flag())) ||
                    state(z80Flag).x86Flag() == x86::Flag::Overflow)
                    continue;
                a.mov(overflowX86Reg, x86Reg);
                a.rol(overflowX86Reg, (u(z80Flag) - u(state(z80Flag).x86Flag())) & 7);
                a.and_(overflowX86Reg, Support::bitMask(u(z80Flag)));
                a.or_(x86::gpb_lo(mat(z80::Reg::AF, true)), overflowX86Reg);
            }
        }
        flush(z80::Reg::AF);
    }

    void flush(x86::Flag x86Flag) {
        for (z80::Flag z80Flag{}; z80Flag != z80::Flag::End; z80Flag = next(z80Flag)) {
            if (!state(z80Flag).isInX86Flag() || state(z80Flag).x86Flag() != x86Flag)
                continue;
            assert(0); // uh oh
        }
        state(x86Flag) = {};
    }

    Error setRotAmt(z80::Reg reg, uint8_t target = 0) {
        target &= 31;
        if (state(reg).rotAmt != target) {
            flush(x86::Flag::Carry);
            flush(x86::Flag::Overflow);
            ASMJIT_PROPAGATE(a.rol(x86::gpd(state(reg).x86Reg), (target - state(reg).rotAmt) & 31));
        }
        state(reg).rotAmt = target;
        return kErrorOk;
    }

    x86::Gp access(z80::Reg z80Reg, bool high, bool dirty = false) {
        auto x86Reg = mat(z80Reg, dirty);
        uint8_t pos = (state(z80Reg).rotAmt + (high << 3)) & 31;
        if (pos && !(pos == 8 && x86::hasHi(x86Reg))) {
            setRotAmt(z80Reg, (high && !x86::hasHi(x86Reg)) << 3);
            pos = (state(z80Reg).rotAmt + (high << 3)) & 31;
        }
        assert(!pos || (pos == 8 && x86::hasHi(x86Reg)));
        return x86::gpb(x86Reg, pos);
    }

    void ensureZ80CarryInX86Carry() {
        switch (state(z80::Flag::Carry).kind()) {
            case FlagState::Kind::Unknown:
                assert(0);
            case FlagState::Kind::InZ80Flag:
                a.bt(access(z80::Reg::AF, false), u(state(z80::Flag::Carry).z80Flag()));
                break;
            case FlagState::Kind::InX86Flag:
                assert(state(z80::Flag::Carry).x86Flag() == x86::Flag::Carry);
                break;
            case FlagState::Kind::Constant:
                a.emit(state(z80::Flag::Carry).constantValue() ? x86::Inst::kIdStc
                                                               : x86::Inst::kIdClc);
                break;
        }
    }

    void incdec(bool dec, z80::Reg z80Reg, bool high) {
        assert((!state(z80::Flag::Carry).isInZ80Flag() ||
                state(z80::Flag::Carry).z80Flag() == z80::Flag::Carry) &&
               (!state(z80::Flag::Carry).isInX86Flag() ||
                state(z80::Flag::Carry).x86Flag() == x86::Flag::Carry));
        a.emit(dec ? x86::Inst::kIdDec : x86::Inst::kIdInc, access(z80Reg, high, true));
        // Update Z80 flags
        state(z80::Flag::AddSub) = dec;
        state(z80::Flag::ParityOverflow) = x86::Flag::Overflow;
        state(z80::Flag::HalfCarry) = x86::Flag::AuxCarry;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = x86::Flag::AuxCarry;
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = x86::Flag::Overflow;
    }

    void rota(bool carry, bool left) {
        if (!carry)
            ensureZ80CarryInX86Carry();
        a.emit(carry ? left ? x86::Inst::kIdRol : x86::Inst::kIdRor
                     : left ? x86::Inst::kIdRcl : x86::Inst::kIdRcr,
               access(z80::Reg::AF, true, true), 1);
        // Update Z80 flags
        state(z80::Flag::Carry) = x86::Flag::Carry;
        state(z80::Flag::AddSub) = false;
        state(z80::Flag::HalfCarry) = false;
        // Update X86 flags
        state(x86::Flag::Carry) = x86::Flag::Carry;
        state(x86::Flag::Overflow) = {};
    }

    void cpl() {
        a.not_(access(z80::Reg::AF, true, true));
        // Update Z80 flags
        state(z80::Flag::AddSub) = true;
        state(z80::Flag::HalfCarry) = true;
    }

    void scf() {
        // Update Z80 flags
        state(z80::Flag::Carry) = true;
        state(z80::Flag::AddSub) = false;
        state(z80::Flag::HalfCarry) = false;
    }

    void ld(z80::Reg z80Dst, bool highDst, z80::Reg z80Src, bool highSrc) {
        assert(z80Dst != z80Src);
        a.mov(access(z80Dst, highDst, true), access(z80Src, highSrc));
    }

    void ld(z80::Reg z80DstSrc, bool highToLow) {
        auto x86DstSrc = mat(z80DstSrc, true);
        if (!x86::hasHi(x86DstSrc)) {
            asm("int3");
            for (z80::Reg z80Reg = next(z80::Reg::AF); z80Reg != z80::Reg::TEMP; z80Reg = next(z80Reg)) {
                auto x86Reg = x86::Gp::Id(state(z80Reg).x86Reg);
                if (x86::hasHi(x86Reg)) {
                    a.xchg(x86::gpd(x86DstSrc), x86::gpd(x86Reg));
                    std::swap(state(z80DstSrc).x86Reg, state(z80Reg).x86Reg);
                    x86DstSrc = x86Reg;
                    break;
                }
            }
        }
        assert(x86::hasHi(x86DstSrc));
        a.mov(x86::gpb(x86DstSrc, !highToLow), x86::gpb(x86DstSrc, highToLow));
    }

    bool ldi(z80::Reg z80Reg, bool high) {
        uint8_t imm;
        if (!fetch(imm))
            return false;
        a.mov(access(z80Reg, high, true), imm);
        return true;
    }

    void addsubcp(bool sub, bool cp, z80::Reg z80Reg, bool high) {
        a.emit(sub ? cp ? x86::Inst::kIdCmp : x86::Inst::kIdSub : x86::Inst::kIdAdd,
               access(z80::Reg::AF, true, true), access(z80Reg, high));
        // Update Z80 flags
        state(z80::Flag::Carry) = x86::Flag::Carry;
        state(z80::Flag::AddSub) = sub;
        state(z80::Flag::ParityOverflow) = x86::Flag::Overflow;
        state(z80::Flag::HalfCarry) = x86::Flag::AuxCarry;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = x86::Flag::Carry;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = x86::Flag::AuxCarry;
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = x86::Flag::Overflow;
    }

    void adcsbc(bool sbc, z80::Reg z80Reg, bool high) {
        ensureZ80CarryInX86Carry();
        a.emit(sbc ? x86::Inst::kIdSbb : x86::Inst::kIdAdc,
               access(z80::Reg::AF, true, true), access(z80Reg, high));
        // Update Z80 flags
        state(z80::Flag::Carry) = x86::Flag::Carry;
        state(z80::Flag::AddSub) = sbc;
        state(z80::Flag::ParityOverflow) = x86::Flag::Overflow;
        state(z80::Flag::HalfCarry) = x86::Flag::AuxCarry;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = x86::Flag::Carry;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = x86::Flag::AuxCarry;
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = x86::Flag::Overflow;
    }

    void subxorcpaa(bool sub, bool cp) {
        if (!cp)
            a.mov(x86::gpb_hi(mat(z80::Reg::AF, true)), 0);
        // Update Z80 flags
        state(z80::Flag::Carry) = false;
        state(z80::Flag::AddSub) = sub;
        state(z80::Flag::ParityOverflow) = !sub;
        state(z80::Flag::HalfCarry) = false;
        state(z80::Flag::Zero) = true;
        state(z80::Flag::Sign) = false;
    }

    void andxoror(bool half, bool x, z80::Reg z80Reg, bool high) {
        a.emit(half ? x86::Inst::kIdAnd : x ? x86::Inst::kIdXor : x86::Inst::kIdOr,
               access(z80::Reg::AF, true, true), access(z80Reg, high));
        // Update Z80 flags
        state(z80::Flag::Carry) = false;
        state(z80::Flag::AddSub) = false;
        state(z80::Flag::ParityOverflow) = x86::Flag::Parity;
        state(z80::Flag::HalfCarry) = half;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = false;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = {};
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = false;
    }

    void andortstaa(bool half) {
        a.test(x86::gpb_hi(mat(z80::Reg::AF)), x86::gpb_hi(mat(z80::Reg::AF)));
        // Update Z80 flags
        state(z80::Flag::Carry) = false;
        state(z80::Flag::AddSub) = false;
        state(z80::Flag::ParityOverflow) = x86::Flag::Parity;
        state(z80::Flag::HalfCarry) = half;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = false;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = {};
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = false;
    }

    bool addsubcpi(bool sub, bool cp) {
        uint8_t imm;
        if (!fetch(imm))
            return false;
        a.emit(sub ? cp ? x86::Inst::kIdCmp : x86::Inst::kIdSub : x86::Inst::kIdAdd,
               access(z80::Reg::AF, true, true), imm);
        // Update Z80 flags
        state(z80::Flag::Carry) = x86::Flag::Carry;
        state(z80::Flag::AddSub) = sub;
        state(z80::Flag::ParityOverflow) = x86::Flag::Overflow;
        state(z80::Flag::HalfCarry) = x86::Flag::AuxCarry;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = x86::Flag::Carry;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = x86::Flag::AuxCarry;
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = x86::Flag::Overflow;
        return true;
    }

    bool adcsbci(bool sbc) {
        uint8_t imm;
        if (!fetch(imm))
            return false;
        ensureZ80CarryInX86Carry();
        a.emit(sbc ? x86::Inst::kIdSbb : x86::Inst::kIdAdc,
               access(z80::Reg::AF, true, true), imm);
        // Update Z80 flags
        state(z80::Flag::Carry) = x86::Flag::Carry;
        state(z80::Flag::AddSub) = sbc;
        state(z80::Flag::ParityOverflow) = x86::Flag::Overflow;
        state(z80::Flag::HalfCarry) = x86::Flag::AuxCarry;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = x86::Flag::Carry;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = x86::Flag::AuxCarry;
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = x86::Flag::Overflow;
        return true;
    }

    bool andxorori(bool half, bool x) {
        uint8_t imm;
        if (!fetch(imm))
            return false;
        a.emit(half ? x86::Inst::kIdAnd : x ? x86::Inst::kIdXor : x86::Inst::kIdOr,
               access(z80::Reg::AF, true, true), imm);
        // Update Z80 flags
        state(z80::Flag::Carry) = false;
        state(z80::Flag::AddSub) = false;
        state(z80::Flag::ParityOverflow) = x86::Flag::Parity;
        state(z80::Flag::HalfCarry) = half;
        state(z80::Flag::Zero) = x86::Flag::Zero;
        state(z80::Flag::Sign) = x86::Flag::Sign;
        // Update X86 flags
        state(x86::Flag::Carry) = false;
        state(x86::Flag::Parity) = x86::Flag::Parity;
        state(x86::Flag::AuxCarry) = {};
        state(x86::Flag::Zero) = x86::Flag::Zero;
        state(x86::Flag::Sign) = x86::Flag::Sign;
        state(x86::Flag::Overflow) = false;
        return true;
    }

    bool fetch(uint8_t &value) {
        uint32_t address = pc++ & 0xFFFFFF;
        if (address < 0x800000) {
            if (address <= flash.mask) {
                cycles += flash.waitStates;
            } else {
                address &= flash.mask;
                cycles += 258;
            }
            value = mem.flash.block[address];
        } else if (address < 0xD00000 || address >= 0xD00000 + SIZE_RAM) {
            return false;
        } else {
            address &= 0x07FFFF;
            value = mem.ram.block[address];
            cycles += 4;
        }
        return true;
    }

    bool genInst() {
        uint8_t value;
        if (!fetch(value)) return false;
        switch (value) {
            case 0000:                                               break; // NOP
            case 0004:           incdec(false, z80::Reg::BC,  true); break; // INC B
            case 0005:           incdec( true, z80::Reg::BC,  true); break; // DEC B
            case 0006:                     return ldi(z80::Reg::BC,  true); // LD B,n
            case 0007:                           rota( true,  true); break; // RLCA
            case 0014:           incdec(false, z80::Reg::BC, false); break; // INC C
            case 0015:           incdec( true, z80::Reg::BC, false); break; // DEC C
            case 0016:                     return ldi(z80::Reg::BC, false); // LD C,n
            case 0017:                           rota( true, false); break; // RRCA
            case 0024:           incdec(false, z80::Reg::DE,  true); break; // INC D
            case 0025:           incdec( true, z80::Reg::DE,  true); break; // DEC D
            case 0026:                     return ldi(z80::Reg::DE,  true); // LD D,n
            case 0027:                           rota(false,  true); break; // RLA
            case 0034:           incdec(false, z80::Reg::DE, false); break; // INC E
            case 0035:           incdec( true, z80::Reg::DE, false); break; // DEC E
            case 0036:                     return ldi(z80::Reg::DE, false); // LD E,n
            case 0037:                           rota(false, false); break; // RRA
            case 0044:           incdec(false, z80::Reg::HL,  true); break; // INC H
            case 0045:           incdec( true, z80::Reg::HL,  true); break; // DEC H
            case 0046:                     return ldi(z80::Reg::HL,  true); // LD H,n
            case 0054:           incdec(false, z80::Reg::HL, false); break; // INC L
            case 0055:           incdec( true, z80::Reg::HL, false); break; // DEC L
            case 0056:                     return ldi(z80::Reg::HL, false); // LD L,n
            case 0057:                                        cpl(); break; // CPL
            case 0067:                                        scf(); break; // SCF
            case 0074:           incdec(false, z80::Reg::AF,  true); break; // INC A
            case 0075:           incdec( true, z80::Reg::AF,  true); break; // DEC A
            case 0076:                     return ldi(z80::Reg::AF,  true); // LD A,n
            case 0100:              ++unhandledValues[value]; return false; // .SIS
            case 0101:                      ld(z80::Reg::BC, false); break; // LD B,C
            case 0102: ld(z80::Reg::BC,  true, z80::Reg::DE,  true); break; // LD B,D
            case 0103: ld(z80::Reg::BC,  true, z80::Reg::DE, false); break; // LD B,E
            case 0104: ld(z80::Reg::BC,  true, z80::Reg::HL,  true); break; // LD B,H
            case 0105: ld(z80::Reg::BC,  true, z80::Reg::HL, false); break; // LD B,L
            case 0107: ld(z80::Reg::BC,  true, z80::Reg::AF,  true); break; // LD B,A
            case 0110:                      ld(z80::Reg::BC,  true); break; // LD C,B
            case 0111:              ++unhandledValues[value]; return false; // .LIS
            case 0112: ld(z80::Reg::BC, false, z80::Reg::DE,  true); break; // LD C,D
            case 0113: ld(z80::Reg::BC, false, z80::Reg::DE, false); break; // LD C,E
            case 0114: ld(z80::Reg::BC, false, z80::Reg::HL,  true); break; // LD C,H
            case 0115: ld(z80::Reg::BC, false, z80::Reg::HL, false); break; // LD C,L
            case 0117: ld(z80::Reg::BC, false, z80::Reg::AF,  true); break; // LD C,A
            case 0120: ld(z80::Reg::DE,  true, z80::Reg::BC,  true); break; // LD D,B
            case 0121: ld(z80::Reg::DE,  true, z80::Reg::BC, false); break; // LD D,C
            case 0122:              ++unhandledValues[value]; return false; // .SIL
            case 0123:                      ld(z80::Reg::DE, false); break; // LD D,E
            case 0124: ld(z80::Reg::DE,  true, z80::Reg::HL,  true); break; // LD D,H
            case 0125: ld(z80::Reg::DE,  true, z80::Reg::HL, false); break; // LD D,L
            case 0127: ld(z80::Reg::DE,  true, z80::Reg::AF,  true); break; // LD D,A
            case 0130: ld(z80::Reg::DE, false, z80::Reg::BC,  true); break; // LD E,B
            case 0131: ld(z80::Reg::DE, false, z80::Reg::BC, false); break; // LD E,C
            case 0132:                      ld(z80::Reg::DE,  true); break; // LD E,D
            case 0133:              ++unhandledValues[value]; return false; // .LIL
            case 0134: ld(z80::Reg::DE, false, z80::Reg::HL,  true); break; // LD E,H
            case 0135: ld(z80::Reg::DE, false, z80::Reg::HL, false); break; // LD E,L
            case 0137: ld(z80::Reg::DE, false, z80::Reg::AF,  true); break; // LD E,A
            case 0140: ld(z80::Reg::HL,  true, z80::Reg::BC,  true); break; // LD H,B
            case 0141: ld(z80::Reg::HL,  true, z80::Reg::BC, false); break; // LD H,C
            case 0142: ld(z80::Reg::HL,  true, z80::Reg::DE,  true); break; // LD H,D
            case 0143: ld(z80::Reg::HL,  true, z80::Reg::DE, false); break; // LD H,E
            case 0144:                                               break; // LD H,H
            case 0145:                      ld(z80::Reg::HL, false); break; // LD H,L
            case 0147: ld(z80::Reg::HL,  true, z80::Reg::AF,  true); break; // LD H,A
            case 0150: ld(z80::Reg::HL, false, z80::Reg::BC,  true); break; // LD L,B
            case 0151: ld(z80::Reg::HL, false, z80::Reg::BC, false); break; // LD L,C
            case 0152: ld(z80::Reg::HL, false, z80::Reg::DE,  true); break; // LD L,D
            case 0153: ld(z80::Reg::HL, false, z80::Reg::DE, false); break; // LD L,E
            case 0154:                      ld(z80::Reg::HL,  true); break; // LD L,H
            case 0155:                                               break; // LD L,L
            case 0157: ld(z80::Reg::HL, false, z80::Reg::AF,  true); break; // LD L,A
            case 0170: ld(z80::Reg::AF,  true, z80::Reg::BC,  true); break; // LD A,B
            case 0171: ld(z80::Reg::AF,  true, z80::Reg::BC, false); break; // LD A,C
            case 0172: ld(z80::Reg::AF,  true, z80::Reg::DE,  true); break; // LD A,D
            case 0173: ld(z80::Reg::AF,  true, z80::Reg::DE, false); break; // LD A,E
            case 0174: ld(z80::Reg::AF,  true, z80::Reg::HL,  true); break; // LD A,H
            case 0175: ld(z80::Reg::AF,  true, z80::Reg::HL, false); break; // LD A,L
            case 0177:                                               break; // LD A,A
            case 0200:  addsubcp(false, false, z80::Reg::BC,  true); break; // ADD A,B
            case 0201:  addsubcp(false, false, z80::Reg::BC, false); break; // ADD A,C
            case 0202:  addsubcp(false, false, z80::Reg::DE,  true); break; // ADD A,D
            case 0203:  addsubcp(false, false, z80::Reg::DE, false); break; // ADD A,E
            case 0204:  addsubcp(false, false, z80::Reg::HL,  true); break; // ADD A,H
            case 0205:  addsubcp(false, false, z80::Reg::HL, false); break; // ADD A,L
            case 0207:  addsubcp(false, false, z80::Reg::AF,  true); break; // ADD A,A
            case 0210:           adcsbc(false, z80::Reg::BC,  true); break; // ADC A,B
            case 0211:           adcsbc(false, z80::Reg::BC, false); break; // ADC A,C
            case 0212:           adcsbc(false, z80::Reg::DE,  true); break; // ADC A,D
            case 0213:           adcsbc(false, z80::Reg::DE, false); break; // ADC A,E
            case 0214:           adcsbc(false, z80::Reg::HL,  true); break; // ADC A,H
            case 0215:           adcsbc(false, z80::Reg::HL, false); break; // ADC A,L
            case 0217:           adcsbc(false, z80::Reg::AF,  true); break; // ADC A,A
            case 0220:  addsubcp( true, false, z80::Reg::BC,  true); break; // SUB A,B
            case 0221:  addsubcp( true, false, z80::Reg::BC, false); break; // SUB A,C
            case 0222:  addsubcp( true, false, z80::Reg::DE,  true); break; // SUB A,D
            case 0223:  addsubcp( true, false, z80::Reg::DE, false); break; // SUB A,E
            case 0224:  addsubcp( true, false, z80::Reg::HL,  true); break; // SUB A,H
            case 0225:  addsubcp( true, false, z80::Reg::HL, false); break; // SUB A,L
            case 0227:                     subxorcpaa( true, false); break; // SUB A,A
            case 0230:           adcsbc( true, z80::Reg::BC,  true); break; // SBC A,B
            case 0231:           adcsbc( true, z80::Reg::BC, false); break; // SBC A,C
            case 0232:           adcsbc( true, z80::Reg::DE,  true); break; // SBC A,D
            case 0233:           adcsbc( true, z80::Reg::DE, false); break; // SBC A,E
            case 0234:           adcsbc( true, z80::Reg::HL,  true); break; // SBC A,H
            case 0235:           adcsbc( true, z80::Reg::HL, false); break; // SBC A,L
            case 0237:           adcsbc( true, z80::Reg::AF,  true); break; // SBC A,A
            case 0240:  andxoror( true, false, z80::Reg::BC,  true); break; // AND A,B
            case 0241:  andxoror( true, false, z80::Reg::BC, false); break; // AND A,C
            case 0242:  andxoror( true, false, z80::Reg::DE,  true); break; // AND A,D
            case 0243:  andxoror( true, false, z80::Reg::DE, false); break; // AND A,E
            case 0244:  andxoror( true, false, z80::Reg::HL,  true); break; // AND A,H
            case 0245:  andxoror( true, false, z80::Reg::HL, false); break; // AND A,L
            case 0247:                            andortstaa( true); break; // AND A,A
            case 0250:  andxoror(false,  true, z80::Reg::BC,  true); break; // XOR A,B
            case 0251:  andxoror(false,  true, z80::Reg::BC, false); break; // XOR A,C
            case 0252:  andxoror(false,  true, z80::Reg::DE,  true); break; // XOR A,D
            case 0253:  andxoror(false,  true, z80::Reg::DE, false); break; // XOR A,E
            case 0254:  andxoror(false,  true, z80::Reg::HL,  true); break; // XOR A,H
            case 0255:  andxoror(false,  true, z80::Reg::HL, false); break; // XOR A,L
            case 0257:                     subxorcpaa(false, false); break; // XOR A,A
            case 0260:  andxoror(false, false, z80::Reg::BC,  true); break; // OR A,B
            case 0261:  andxoror(false, false, z80::Reg::BC, false); break; // OR A,C
            case 0262:  andxoror(false, false, z80::Reg::DE,  true); break; // OR A,D
            case 0263:  andxoror(false, false, z80::Reg::DE, false); break; // OR A,E
            case 0264:  andxoror(false, false, z80::Reg::HL,  true); break; // OR A,H
            case 0265:  andxoror(false, false, z80::Reg::HL, false); break; // OR A,L
            case 0267:                            andortstaa(false); break; // OR A,A
            case 0270:  addsubcp( true,  true, z80::Reg::BC,  true); break; // CP A,B
            case 0271:  addsubcp( true,  true, z80::Reg::BC, false); break; // CP A,C
            case 0272:  addsubcp( true,  true, z80::Reg::DE,  true); break; // CP A,D
            case 0273:  addsubcp( true,  true, z80::Reg::DE, false); break; // CP A,E
            case 0274:  addsubcp( true,  true, z80::Reg::HL,  true); break; // CP A,H
            case 0275:  addsubcp( true,  true, z80::Reg::HL, false); break; // CP A,L
            case 0277:                     subxorcpaa( true,  true); break; // CP A,A
            case 0306:                      return addsubcpi(false, false); // ADD A,n
            case 0316:                               return adcsbci(false); // ADC A,n
            case 0326:                      return addsubcpi( true, false); // SUB A,n
            case 0336:                               return adcsbci( true); // SBC A,n
            case 0346:                      return andxorori( true, false); // AND A,n
            case 0356:                      return andxorori(false,  true); // XOR A,n
            case 0366:                      return andxorori(false, false); // OR A,n
            case 0376:                      return addsubcpi( true,  true); // CP A,n
            case 0355: { // ED
                if (!fetch(value)) return false;
                switch (value) {
                    case 0074:                    andortstaa( true); break; // TST A,A
                    default:        ++unhandledValues[value]; return false;
                }
                break;
            }
            default:                ++unhandledValues[value]; return false;
        }
        return true;
    }

    bool gen() {
        prolog();
        do {
            block.end = pc;
            block.cycles = cycles;
        } while (genInst() && cycles <= 0x80);
        if ((block.size = block.end - block.start)) {
            for (uint32_t address = block.start; address < block.end; address++)
                fetched[address & 0xFFFFFF] = true;
            flush();
            a.mov(x86::eax, block.end);
            a.ret();
            a.bind(eventLabel);
            a.add(x86::esi, -block.cycles);
            a.mov(x86::eax, kEventFlag | block.start);
            a.ret();
#if 0
            Label patchPoint = a.newLabel();
            a.bind(patchPoint);
            a.long_().mov(a.zax(), imm(jitBind));
            a.call(a.zax());
            a.ud2();
#endif
            fixup();
            runtime.add(&block.entry, &code);
#if 0
            patchPoints[static_cast<void *>(reinterpret_cast<char *>(block.entry) + code.labelOffset(patchPoint))] = block.end;
#endif
            blocks[block.start] = &block;
            //if (void *p = phys_mem_ptr(block->start & 0xFFFFFF, block->size))
            //    logger.logBinary(p, block->size);
            //if (block->size) {
            //    puts(logger.data());
            //    fflush(stdout);
            //}
            return true;
        } else {
            blockAlloc.release(&block, sizeof(CodeBlock));
            return false;
        }
    }
};

bool gen(uint32_t pc) {
    CodeBlock *block = blockAlloc.allocT<CodeBlock>();
    if (!block)
        return false;
    return Gen(*block, pc).gen();
}

void jitPatch(void *target) {
    uint32_t address = *patchPoints.find(target);
    gen(address);
}

}

void jitFlush() {
    runtime.reset(Globals::kResetHard);
    fetched.clear();
    blocks.clear();
    blockZone.reset();
    blockAlloc.reset(&blockZone);

    CodeHolder code;
    x86::Assembler a;
    init(code, a);
    a.push(a.zbx());
    a.call(a.zdx());
    //a.and_(x86::eax, 0xFFFFFF);
    a.add(x86::esi, a.ptr_zdi(offsetof(eZ80cpu_t, next)));
    //a.mov(a.ptr_zdi(offsetof(eZ80cpu_t, registers.PC)), x86::eax);
    a.mov(a.ptr_zdi(offsetof(eZ80cpu_t, cycles)), x86::esi);
    a.pop(a.zbx());
    a.ret();
    runtime.add(&dispatch, &code);
}

void jitReportWrite(uint32_t address, uint8_t value) {
    if (!fetched[address & 0xFFFFFF]) {
        return;
    }
    jitFlush();
}

bool jitTryExecute() {
    uint32_t address = cpu.ADL ? kAdlFlag | (cpu.registers.PC & 0xFFFFFF)
                               : cpu.registers.MBASE << 16 | (cpu.registers.PC & 0xFFFF);
    do {
        if (auto block = blocks.find(address)) {
            uint32_t address = dispatch(&cpu, cpu.cycles - cpu.next, (*block)->entry);
            ++blocksExecuted;
            cpu_flush(address & 0xFFFFFF, address & kAdlFlag);
            return true;
        }
        if (fetched[address & 0xFFFFFF]) {
            return false;
        }
    } while (gen(address));
    return false;
}

#endif
