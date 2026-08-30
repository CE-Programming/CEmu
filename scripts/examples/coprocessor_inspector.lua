-- Inspect the Python ARM coprocessor without changing its execution state.
if not coproc.present() then
    print("No ARM coprocessor is active for this calculator")
    return
end

local bootloader = coproc.bootloader()
print(string.format("ARM bootloader: %s (%s)",
                    bootloader.description, bootloader.type))
print(string.format("Vectors: initial SP=%08X reset handler=%08X",
                    coproc.readWord(0), coproc.readWord(4)))

local state = coproc.state()
local registers = state.registers
print(string.format("PC=%08X SP=%08X LR=%08X cycles=%d limit=%d sleeping=%s",
                    registers.pc, registers.sp, registers.lr,
                    state.cycles, state.cycleLimit, tostring(state.sleeping)))

for first = 0, 12, 4 do
    local values = {}
    for index = first, math.min(first + 3, 12) do
        values[#values + 1] = string.format("r%d=%08X", index, registers["r" .. index])
    end
    print(table.concat(values, " "))
end

local flags = state.flags
print(string.format("N=%s Z=%s C=%s V=%s PRIMASK=%s exception=%s WFI=%s",
                    tostring(flags.negative), tostring(flags.zero),
                    tostring(flags.carry), tostring(flags.overflow),
                    tostring(flags.primask), tostring(flags.inException),
                    tostring(flags.waitingForInterrupt)))
print(string.format("VTOR=%08X ICSR=%08X SysTick=%08X/%08X",
                    state.scb.vectorTable, state.scb.interruptControl,
                    state.systick.current, state.systick.reload))
