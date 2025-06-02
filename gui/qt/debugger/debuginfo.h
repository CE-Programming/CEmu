#ifndef DEBUGINFO_H
#define DEBUGINFO_H

/* Enable math constants on MSVC */
#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace debuginfo {
namespace detail {

constexpr auto BitsPerByte = std::numeric_limits<unsigned char>::digits;

template<bool Signed, std::size_t Bits, std::size_t... Indices>
class [[gnu::packed]] UnalignedLittleEndianIntegerImpl {
    using Byte = unsigned char;
    using IntMax = std::conditional_t<Signed, std::intmax_t, std::uintmax_t>;

public:
    std::array<Byte, sizeof...(Indices)> m_value;
    constexpr UnalignedLittleEndianIntegerImpl() = default;
    constexpr UnalignedLittleEndianIntegerImpl(IntMax value)
        : m_value{ Byte(value >> Indices * BitsPerByte)... } {}
    template<bool ValSigned, std::size_t ValBits, std::size_t... ValIndices>
    constexpr UnalignedLittleEndianIntegerImpl(
            UnalignedLittleEndianIntegerImpl<ValSigned, ValBits, ValIndices...> value)
        : UnalignedLittleEndianIntegerImpl{IntMax(value)} {}

    constexpr UnalignedLittleEndianIntegerImpl &operator++() noexcept {
        return *this = *this + 1;
    }
    constexpr UnalignedLittleEndianIntegerImpl &operator++(int) noexcept {
        auto result = *this;
        ++*this;
        return result;
    }
    constexpr UnalignedLittleEndianIntegerImpl &operator+=(IntMax value) noexcept {
        return *this = *this + value;
    }
    constexpr UnalignedLittleEndianIntegerImpl &operator--() noexcept {
        return *this = *this - 1;
    }
    constexpr UnalignedLittleEndianIntegerImpl &operator--(int) noexcept {
        auto result = *this;
        --*this;
        return result;
    }
    constexpr UnalignedLittleEndianIntegerImpl &operator-=(IntMax value) noexcept {
        return *this = *this - value;
    }

    constexpr operator IntMax() const noexcept {
        constexpr IntMax extend = std::numeric_limits<IntMax>::digits +
            std::numeric_limits<IntMax>::is_signed - Bits;
        return ((IntMax(m_value[Indices]) << Indices * BitsPerByte) | ...) << extend >> extend;
    }
};

template<bool Signed, std::size_t Bits, std::size_t... Indices>
UnalignedLittleEndianIntegerImpl<Signed, Bits, Indices...>
UnalignedLittleEndianIntegerHelper(std::index_sequence<Indices...>);

template<bool Signed, std::size_t Bits>
using UnalignedLittleEndianInteger =
    decltype(UnalignedLittleEndianIntegerHelper<Signed, Bits>(
                     std::make_index_sequence<(Bits + CHAR_BIT - 1) / CHAR_BIT>{}));

template<typename Integer> class SizedInteger {
public:
    template<typename Int> constexpr SizedInteger(
            Int value, std::enable_if_t<std::numeric_limits<Int>::is_integer
            && std::is_signed<Int>::value == std::is_signed<Integer>::value, std::size_t> width
            = std::numeric_limits<Int>::digits + std::numeric_limits<Int>::is_signed) noexcept
        : m_value(value), m_width(width) {}
    template<typename Enum> explicit constexpr SizedInteger(
            Enum value, std::enable_if_t<std::is_enum<Enum>::value
            && std::is_signed<std::underlying_type_t<Enum>>::value ==
            std::is_signed<Integer>::value, std::size_t> width
            = std::numeric_limits<std::underlying_type_t<Enum>>::digits
            + std::numeric_limits<std::underlying_type_t<Enum>>::is_signed) noexcept
        : m_value(Integer(value)), m_width(width) {}
    constexpr std::size_t width() const noexcept { return m_width; }
    constexpr Integer get() const noexcept { return m_value; }
    constexpr operator Integer() const noexcept { return m_value; }

private:
    Integer m_value;
    std::size_t m_width;
};

} // end namespace detail

enum class _Void : unsigned char {};
using _Sbyte = detail::UnalignedLittleEndianInteger<true,   8>;
using _Byte  = detail::UnalignedLittleEndianInteger<false,  8>;
using _Shalf = detail::UnalignedLittleEndianInteger<true,  16>;
using _Half  = detail::UnalignedLittleEndianInteger<false, 16>;
using _Addr  = detail::UnalignedLittleEndianInteger<false, 24>;
using _Off   = detail::UnalignedLittleEndianInteger<false, 32>;
using _Sword = detail::UnalignedLittleEndianInteger<true,  32>;
using _Word  = detail::UnalignedLittleEndianInteger<false, 32>;
using _Slong = detail::UnalignedLittleEndianInteger<true,  64>;
using _Long  = detail::UnalignedLittleEndianInteger<false, 64>;

static_assert(std::is_trivial_v<_Addr> && alignof(_Addr) == 1 && sizeof(_Addr) == 3,
              "_Addr should be trivial, unaligned, and 3 bytes");

template<typename _Type> class _View {
public:
    using element_type           = _Type;
    using value_type             = std::decay_t<std::remove_cv_t<element_type>>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using pointer                = element_type *;
    using const_pointer          = const element_type *;
    using reference              = element_type &;
    using const_reference        = const element_type &;
    using iterator               = pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_iterator         = const_pointer;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static const _View _S_empty;

    constexpr _View() noexcept : _M_size(0), _M_ptr(nullptr) {}
    constexpr _View(iterator __first, size_type __count) noexcept : _M_size(__count), _M_ptr(__first) {}
    constexpr _View(iterator __first, iterator __last) noexcept : _M_size(std::distance(__first, __last)), _M_ptr(__first) {}
    template<size_type _Size> _View(element_type (&__arr)[_Size]) noexcept : _M_size(_Size), _M_ptr(__arr) {}
    ~_View() noexcept = default;
    constexpr _View(const _View &) noexcept = default;
    _View &operator=(const _View &) noexcept = default;

    constexpr iterator               begin()   const noexcept { return _M_ptr;           }
    constexpr iterator               end()     const noexcept { return _M_ptr + _M_size; }
    constexpr const_iterator         cbegin()  const noexcept { return _M_ptr;           }
    constexpr const_iterator         cend()    const noexcept { return _M_ptr + _M_size; }
    constexpr reverse_iterator       rbegin()  const noexcept { return end();            }
    constexpr reverse_iterator       rend()    const noexcept { return begin();          }
    constexpr const_reverse_iterator crbegin() const noexcept { return cend();           }
    constexpr const_reverse_iterator crend()   const noexcept { return cbegin();         }

    constexpr reference front() const noexcept { assert(!empty()); return _M_ptr[0]; }
    constexpr value_type front_value() const noexcept { if (empty()) return {}; return front(); }
    constexpr reference back() const noexcept { assert(!empty()); return _M_ptr[_M_size - 1]; }
    constexpr value_type back_value() const noexcept { if (empty()) return {}; return back(); }
    constexpr reference operator[](size_type __pos) const noexcept { assert(__pos < _M_size); return _M_ptr[__pos]; }
    constexpr value_type value(size_type __pos) const noexcept { if (__pos < _M_size) return (*this)[__pos]; return {}; }
    constexpr pointer data() const noexcept { return _M_ptr; }

    constexpr size_type size() const noexcept { return _M_size; }
    constexpr size_type size_bytes() const noexcept { return _M_size * size_type(sizeof(element_type)); }
    constexpr bool empty() const noexcept { return !_M_size; }

    constexpr _View first(size_type __count) const noexcept { return { _M_ptr, std::min(_M_size, __count) }; }
    constexpr _View last(size_type __count) const noexcept { return { _M_ptr + (_M_size - std::min(_M_size, __count)), std::min(_M_size, __count) }; }
    constexpr _View subview(size_type __offset, size_type __count = std::numeric_limits<size_type>::max()) const noexcept { return { _M_ptr + __offset, std::min(_M_size - std::min(_M_size, __offset), __count) }; }

    template<typename _Tp> constexpr _View<_Tp> bitcast() const noexcept { return { reinterpret_cast<typename _View<_Tp>::pointer>(_M_ptr), size_bytes() / size_type(sizeof(_Tp)) }; }

    constexpr bool operator==(const _View &__view) const noexcept {
        return std::equal(begin(), end(), __view.begin(), __view.end());
    }
    constexpr bool operator!=(const _View &__view) const noexcept { return !(*this == __view); }
    constexpr bool operator< (const _View &__view) const noexcept {
        return std::lexicographical_compare(begin(), end(), __view.begin(), __view.end());
    }
    constexpr bool operator> (const _View &__view) const noexcept { return __view < *this; }
    constexpr bool operator<=(const _View &__view) const noexcept { return !(*this < __view); }
    constexpr bool operator>=(const _View &__view) const noexcept { return !(__view < *this); }

    constexpr void drop_first(size_type __count) noexcept {
        __count = std::min(_M_size, __count);
        _M_size -= __count;
        _M_ptr += __count;
    }
    constexpr _View take_first(size_type __count) noexcept {
        _View __res = first(__count);
        drop_first(__count);
        return __res;
    }
    constexpr void drop_last(size_type __count) noexcept {
        _M_size -= std::min(_M_size, __count);
    }
    constexpr _View take_last(size_type __count) noexcept {
        _View __res = last(__count);
        drop_last(__count);
        return __res;
    }

    constexpr _View ltrim(value_type __trim) const noexcept {
        _View __res(*this);
        while (!__res.empty() && __res.front() == __trim) {
            __res.drop_first(1);
        }
        return __res;
    }
    constexpr _View rtrim(value_type __trim) const noexcept {
        _View __res(*this);
        while (!__res.empty() && __res.back() == __trim) {
            __res.drop_last(1);
        }
        return __res;
    }

    template<typename _Tp> constexpr _Long partial() const noexcept {
        _Tp __res{};
        std::memcpy(&__res, _M_ptr, std::min(_M_size, size_type(sizeof(_Tp))));
        return __res;
    }
    template<typename _Tp> constexpr _Long complete() const noexcept {
        _Tp __res{};
        if (size_bytes() >= sizeof(_Tp)) {
            std::memcpy(&__res, _M_ptr, std::min(_M_size, size_type(sizeof(_Tp))));
        }
        return __res;
    }

private:
    size_type _M_size;
    pointer _M_ptr;
};
template<typename _Type> const _View<_Type> _View<_Type>::_S_empty;

using Data = _View<const _Void>;
using _Str = _View<const char>;
using _OffView = _View<const _Off>;
using _AddrView = _View<const _Addr>;

template<std::size_t _MaxParts> class _Path {
    using _Parts = std::array<const _Str, _MaxParts>;

public:
    static const _Path _S_null;
    static constexpr _Str::value_type sep =
#ifdef _WIN32
        '\\';
#else
        '/';
#endif
    static constexpr bool is_sep(_Str::value_type __c) noexcept { return __c == sep || __c == '/'; }
    static constexpr bool is_abs(_Str __part) noexcept {
        return is_sep(__part.value(0))
#ifdef _WIN32
            || (__part.value(1) == ':' && is_sep(__part.value(2)))
#endif
            ;
    }

    using value_type             = typename _Parts::value_type;
    using size_type              = typename _Parts::size_type;
    using difference_type        = typename _Parts::difference_type;
    using reference              = typename _Parts::reference;
    using const_reference        = typename _Parts::const_reference;
    using pointer                = typename _Parts::pointer;
    using const_pointer          = typename _Parts::const_pointer;
    using iterator               = typename _Parts::iterator;
    using const_iterator         = typename _Parts::const_iterator;
    using reverse_iterator       = typename _Parts::reverse_iterator;
    using const_reverse_iterator = typename _Parts::const_reverse_iterator;

    constexpr _Path() noexcept = default;
    constexpr _Path(std::initializer_list<_Str> __parts) noexcept {
        auto __it = _M_parts.begin();
        for (auto __part : __parts) {
            if (__part.rtrim('\0').empty()) continue;
            if (is_abs(__part)) {
                __it = _M_parts.begin();
            }
            if (__it == _M_parts.end()) {
                __it = _M_parts.begin();
                break;
            }
            *__it++ = __part;
        }
        while (__it != _M_parts.end()) {
            *__it++ = {};
        }
    }

    constexpr const_reference at(size_type __pos) const { return _M_parts[__pos]; }
    constexpr const_reference operator[](size_type __pos) const noexcept {
        if (__pos < _M_parts.size())
            return _M_parts[__pos];
        return _Str::_S_empty;
    }
    constexpr const_reference front() const noexcept { return (*this)[0]; }
    constexpr const_reference back()  const noexcept { return (*this)[size()]; }
    pointer data() const noexcept { return _M_parts.data(); }

    auto begin()   const noexcept { return _M_parts.begin(); }
    auto end()     const noexcept {
        return std::find_if(_M_parts.begin(), _M_parts.end(),
                            [](const_reference __part) { return __part.empty(); });
    }
    auto cbegin()  const noexcept { return const_iterator(begin());          }
    auto cend()    const noexcept { return const_iterator(end());            }
    auto rbegin()  const noexcept { return reverse_iterator(end());          }
    auto rend()    const noexcept { return reverse_iterator(begin());        }
    auto crbegin() const noexcept { return const_reverse_iterator(rbegin()); }
    auto crend()   const noexcept { return const_reverse_iterator(rend());   }

    constexpr bool empty() const noexcept { return _M_parts.front().empty(); }
    constexpr size_type size() const noexcept { return std::distance(begin(), end()); }
    constexpr size_type max_size() const noexcept { return _MaxParts; }
    bool is_abs() const noexcept { return is_abs(front()); }

    constexpr bool operator==(const _Path &__path) const noexcept {
        return std::equal(canon().begin(), canon().end(),
                          __path.canon().begin(), __path.canon().end());
    }
    constexpr bool operator!=(const _Path &__path) const noexcept { return !(*this == __path); }
    constexpr bool operator< (const _Path &__path) const noexcept {
        return std::lexicographical_compare(canon().begin(), canon().end(),
                                            __path.canon().begin(), __path.canon().end());
    }
    constexpr bool operator> (const _Path &__path) const noexcept { return __path < *this; }
    constexpr bool operator<=(const _Path &__path) const noexcept { return !(*this < __path); }
    constexpr bool operator>=(const _Path &__path) const noexcept { return !(__path < *this); }

private:
    class _Canon {
        friend _Path;

        enum class _State : bool { _AtSep, _Default };

        constexpr _Canon(const _Path &__path) noexcept : _M_path(__path) {}

    public:
        using value_type             = _Str::value_type;
        using size_type              = _Str::size_type;
        using difference_type        = _Str::difference_type;
        using reference              = _Str::reference;
        using const_reference        = _Str::const_reference;
        using pointer                = _Str::pointer;
        using const_pointer          = _Str::const_pointer;

        class iterator {
            friend _Canon;

            constexpr auto tie() const noexcept { return std::tie(_M_pos, _M_it, _M_state); }
            constexpr const _Str &part() const noexcept { return _M_path[_M_pos]; }
            constexpr void next() noexcept {
                while (_M_it != part().end() && is_sep(*_M_it)) {
                    ++_M_it;
                    _M_state = _State::_AtSep;
                }
                while (_M_it == part().end() || *_M_it == '\0') {
                    ++_M_pos;
                    _M_it = part().begin();
                    if (part().empty()) {
                        _M_pos = _MaxParts;
                        return;
                    }
                    _M_state = _State::_AtSep;
                }
            }

            constexpr iterator(const _Path &__path, _Path::size_type __pos) noexcept
                : _M_path(__path), _M_pos(__pos), _M_it(part().begin()) { next(); }

        public:
            using value_type        = std::iterator_traits<_Str::iterator>::value_type;
            using difference_type   = std::iterator_traits<_Str::iterator>::difference_type;
            using pointer           = std::iterator_traits<_Str::iterator>::pointer;
            using reference         = std::iterator_traits<_Str::iterator>::reference;
            using iterator_category = std::forward_iterator_tag;

            constexpr iterator() : _M_path(_S_null), _M_pos(), _M_it() {}

            constexpr reference operator*() const noexcept {
                switch (_M_state) {
                    case _State::_AtSep:
                        return sep;
                    case _State::_Default:
                        return *_M_it;
                }
            }
            constexpr pointer operator->() const noexcept { return &*this; }
            constexpr iterator &operator++() noexcept {
                if (std::exchange(_M_state, _State::_Default) == _State::_Default) {
                    ++_M_it;
                    next();
                }
                return *this;
            }
            constexpr iterator operator++(int) noexcept { auto __res(*this); ++*this; return __res; }

            constexpr bool operator==(const iterator &__it) const noexcept { return tie() == __it.tie(); }
            constexpr bool operator!=(const iterator &__it) const noexcept { return tie() != __it.tie(); }
            constexpr bool operator< (const iterator &__it) const noexcept { return tie() <  __it.tie(); }
            constexpr bool operator> (const iterator &__it) const noexcept { return tie() >  __it.tie(); }
            constexpr bool operator<=(const iterator &__it) const noexcept { return tie() <= __it.tie(); }
            constexpr bool operator>=(const iterator &__it) const noexcept { return tie() >= __it.tie(); }

        private:
            const _Path &_M_path;
            _Path::size_type _M_pos;
            _Str::iterator _M_it;
            _State _M_state = _State::_Default;
        };
        using const_iterator = iterator;

        constexpr iterator begin() const noexcept { return { _M_path, 0 }; }
        constexpr iterator end() const noexcept { return { _M_path, _MaxParts }; }
        constexpr const_iterator cbegin() const noexcept { return begin(); }
        constexpr const_iterator cend() const noexcept { return end(); }

    private:
        const _Path &_M_path;
    };

public:
    constexpr _Canon canon() const noexcept { return *this; }

private:
    std::array<_Str, _MaxParts> _M_parts;
};
template<std::size_t _MaxParts> const _Path<_MaxParts> _Path<_MaxParts>::_S_null;
template<std::size_t _MaxParts> const _Str::value_type _Path<_MaxParts>::sep;

} // end namespace debuginfo

namespace std {
template<bool Signed, std::size_t Bits, std::size_t... Indices>
class numeric_limits<
    debuginfo::detail::UnalignedLittleEndianIntegerImpl<Signed, Bits, Indices...>> {
    using Integer = debuginfo::detail::UnalignedLittleEndianIntegerImpl<Signed, Bits, Indices...>;

public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = Signed;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr float_round_style round_style = round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_modulo = !Signed;
    static constexpr int digits = Bits - Signed;
    static constexpr int digits10 = M_LN2 / M_LN10 * digits;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;

    static constexpr Integer min() noexcept {
        return Signed ? ~INTMAX_C(0) << (Bits - 1) : UINTMAX_C(0);
    }
    static constexpr Integer lowest() noexcept {
        return Signed ? ~INTMAX_C(0) << (Bits - 1) : UINTMAX_C(0);
    }
    static constexpr Integer max() noexcept { return ~min(); }
    static constexpr Integer epsilon()       noexcept { return 0; }
    static constexpr Integer round_error()   noexcept { return 0; }
    static constexpr Integer infinity()      noexcept { return 0; }
    static constexpr Integer quiet_NaN()     noexcept { return 0; }
    static constexpr Integer signaling_NaN() noexcept { return 0; }
    static constexpr Integer denorm_min()    noexcept { return 0; }
};
template<bool Signed, std::size_t Bits, std::size_t... Indices>
struct numeric_limits<const          debuginfo::detail::UnalignedLittleEndianIntegerImpl<
                          Signed, Bits, Indices...>>
    : numeric_limits<debuginfo::detail::UnalignedLittleEndianIntegerImpl<
                          Signed, Bits, Indices...>> {};
template<bool Signed, std::size_t Bits, std::size_t... Indices>
struct numeric_limits<      volatile debuginfo::detail::UnalignedLittleEndianIntegerImpl<
                          Signed, Bits, Indices...>>
    : numeric_limits<debuginfo::detail::UnalignedLittleEndianIntegerImpl<
                          Signed, Bits, Indices...>> {};
template<bool Signed, std::size_t Bits, std::size_t... Indices>
struct numeric_limits<const volatile debuginfo::detail::UnalignedLittleEndianIntegerImpl<
                          Signed, Bits, Indices...>>
    : numeric_limits<debuginfo::detail::UnalignedLittleEndianIntegerImpl<
                          Signed, Bits, Indices...>> {};

template<> struct hash<debuginfo::_Str> {
    constexpr std::size_t operator()(debuginfo::_Str __str) const noexcept {
        debuginfo::_Word __hash = 5381;
        for (auto c : __str.rtrim('\0')) {
            __hash = __hash * 33 + debuginfo::_Byte(c);
        }
        return __hash;
    }
};

template<std::size_t _MaxParts> struct hash<debuginfo::_Path<_MaxParts>> {
    constexpr std::size_t operator()(const debuginfo::_Path<_MaxParts> &__path) const noexcept {
        debuginfo::_Word __hash = 5381;
        for (auto c : __path.canon()) {
            __hash = __hash * 33 + debuginfo::_Byte(c);
        }
        return __hash;
    }
};
} // end namespace std

namespace debuginfo {
constexpr _Off _InvalidOff = std::numeric_limits<_Off>::max();
constexpr _Word _InvalidWord = std::numeric_limits<_Word>::max();

namespace elf {
class File;

struct [[gnu::packed]] ProgramHeader {
    _Word p_type;
    _Off  p_offset;
    _Word p_vaddr;
    _Word p_paddr;
    _Word p_filesz;
    _Word p_memsz;
    _Word p_flags;
    _Word p_align;

    Data data(const File &___file) const noexcept;
};

struct [[gnu::packed]] SectionHeader {
    _Word sh_name;
    _Word sh_type;
    _Word sh_flags;
    _Word sh_addr;
    _Off  sh_offset;
    _Word sh_size;
    _Word sh_link;
    _Word sh_info;
    _Word sh_addralign;
    _Word sh_entsize;

    _Str name(const File &__file) const noexcept;
    Data data(const File &___file) const noexcept;
};

class File {
    friend SectionHeader;
    friend ProgramHeader;

    constexpr bool has_header() const noexcept { return _M_data.size_bytes() >= sizeof(Header); }

    template<typename _Header> constexpr _View<_Header> get(
            std::uint32_t __offset, std::uint16_t __size, std::uint16_t __count) const noexcept {
        if (__size != sizeof(_Header)) {
            return {};
        }
        return _M_data.subview(__offset).bitcast<_Header>().first(__count);
    }

public:
    struct Header {
        _Byte  ei_mag[4];
        _Byte  ei_class;
        _Byte  ei_data;
        _Byte  ei_version;
        _Byte  ei_osabi;
        _Byte  ei_pad[8];
        _Half  e_type;
        _Half  e_machine;
        _Word  e_version;
        _Word  e_entry;
        _Off   e_phoff;
        _Off   e_shoff;
        _Word  e_flags;
        _Half  e_ehsize;
        _Half  e_phentsize;
        _Half  e_phnum;
        _Half  e_shentsize;
        _Half  e_shnum;
        _Half  e_shstrndx;

        const static Header ez80;
    };

    explicit File(Data __data) noexcept : _M_data(__data) {}

    constexpr auto &header() const noexcept {
        assert(has_header());
        return _M_data.bitcast<const Header>().front();
    }

    constexpr bool validate(const Header &__expected) const noexcept {
        return has_header() && !std::memcmp(&header(), &__expected, offsetof(Header, e_entry)) &&
            (header().e_flags & __expected.e_flags) == __expected.e_flags &&
            header().e_ehsize == sizeof(Header) && header().e_shstrndx < header().e_shnum;
    }

    constexpr auto program_headers() const noexcept {
        return get<const ProgramHeader>(header().e_phoff, header().e_phentsize, header().e_phnum);
    }

    constexpr auto section_headers() const noexcept {
        return get<const SectionHeader>(header().e_shoff, header().e_shentsize, header().e_shnum);
    }

protected:
    const Data _M_data;
};
} // end namespace elf

namespace dwarf {
enum class Reg : std::uint8_t { BC, DE, HL, AF, IX, IY, SPS, SPL, PC, None };

// Table 7.3: Tag encodings
enum class _Tag : std::uint8_t {
    _None,
    DW_TAG_array_type               = 0x01,
    DW_TAG_class_type               = 0x02,
    DW_TAG_entry_point              = 0x03,
    DW_TAG_enumeration_type         = 0x04,
    DW_TAG_formal_parameter         = 0x05,
    DW_TAG_imported_declaration     = 0x08,
    DW_TAG_label                    = 0x0a,
    DW_TAG_lexical_block            = 0x0b,
    DW_TAG_member                   = 0x0d,
    DW_TAG_pointer_type             = 0x0f,
    DW_TAG_reference_type           = 0x10,
    DW_TAG_compile_unit             = 0x11,
    DW_TAG_string_type              = 0x12,
    DW_TAG_structure_type           = 0x13,
    DW_TAG_subroutine_type          = 0x15,
    DW_TAG_typedef                  = 0x16,
    DW_TAG_union_type               = 0x17,
    DW_TAG_unspecified_parameters   = 0x18,
    DW_TAG_variant                  = 0x19,
    DW_TAG_common_block             = 0x1a,
    DW_TAG_common_inclusion         = 0x1b,
    DW_TAG_inheritance              = 0x1c,
    DW_TAG_inlined_subroutine       = 0x1d,
    DW_TAG_module                   = 0x1e,
    DW_TAG_ptr_to_member_type       = 0x1f,
    DW_TAG_set_type                 = 0x20,
    DW_TAG_subrange_type            = 0x21,
    DW_TAG_with_stmt                = 0x22,
    DW_TAG_access_declaration       = 0x23,
    DW_TAG_base_type                = 0x24,
    DW_TAG_catch_block              = 0x25,
    DW_TAG_const_type               = 0x26,
    DW_TAG_constant                 = 0x27,
    DW_TAG_enumerator               = 0x28,
    DW_TAG_file_type                = 0x29,
    DW_TAG_friend                   = 0x2a,
    DW_TAG_namelist                 = 0x2b,
    DW_TAG_namelist_item            = 0x2c,
    DW_TAG_packed_type              = 0x2d,
    DW_TAG_subprogram               = 0x2e,
    DW_TAG_template_type_parameter  = 0x2f,
    DW_TAG_template_value_parameter = 0x30,
    DW_TAG_thrown_type              = 0x31,
    DW_TAG_try_block                = 0x32,
    DW_TAG_variant_part             = 0x33,
    DW_TAG_variable                 = 0x34,
    DW_TAG_volatile_type            = 0x35,
    DW_TAG_dwarf_procedure          = 0x36,
    DW_TAG_restrict_type            = 0x37,
    DW_TAG_interface_type           = 0x38,
    DW_TAG_namespace                = 0x39,
    DW_TAG_imported_module          = 0x3a,
    DW_TAG_unspecified_type         = 0x3b,
    DW_TAG_partial_unit             = 0x3c,
    DW_TAG_imported_unit            = 0x3d,
    DW_TAG_condition                = 0x3f,
    DW_TAG_shared_type              = 0x40,
    DW_TAG_type_unit                = 0x41,
    DW_TAG_rvalue_reference_type    = 0x42,
    DW_TAG_template_alias           = 0x43,
    DW_TAG_coarray_type             = 0x44,
    DW_TAG_generic_subrange         = 0x45,
    DW_TAG_dynamic_type             = 0x46,
    DW_TAG_atomic_type              = 0x47,
    DW_TAG_call_site                = 0x48,
    DW_TAG_call_site_parameter      = 0x49,
    DW_TAG_skeleton_unit            = 0x4a,
    DW_TAG_immutable_type           = 0x4b,
    // For Internal Use
    _Unknown,
    _Unused
};
enum {
    // Vendor Extensions
    DW_TAG_lo_user                  = 0x4080,
    DW_TAG_GNU_call_site            = 0x4109,
    DW_TAG_GNU_call_site_parameter  = 0x410a,
    DW_TAG_hi_user                  = 0xffff,
};
// Table 7.5: Attribute encodings
enum class _At : std::uint8_t {
    _None,
    DW_AT_sibling                   = 0x01, // reference
    DW_AT_location                  = 0x02, // exprloc, loclist
    DW_AT_name                      = 0x03, // string
    DW_AT_ordering                  = 0x09, // constant
    DW_AT_byte_size                 = 0x0b, // constant, exprloc, reference
    DW_AT_bit_size                  = 0x0d, // constant, exprloc, reference
    DW_AT_stmt_list                 = 0x10, // lineptr
    DW_AT_low_pc                    = 0x11, // address
    DW_AT_high_pc                   = 0x12, // address, constant
    DW_AT_language                  = 0x13, // constant
    DW_AT_discr                     = 0x15, // reference
    DW_AT_discr_value               = 0x16, // constant
    DW_AT_visibility                = 0x17, // constant
    DW_AT_import                    = 0x18, // reference
    DW_AT_string_length             = 0x19, // exprloc, loclist, reference
    DW_AT_common_reference          = 0x1a, // reference
    DW_AT_comp_dir                  = 0x1b, // string
    DW_AT_const_value               = 0x1c, // block, constant, string
    DW_AT_containing_type           = 0x1d, // reference
    DW_AT_default_value             = 0x1e, // constant, reference, flag
    DW_AT_inline                    = 0x20, // constant
    DW_AT_is_optional               = 0x21, // flag
    DW_AT_lower_bound               = 0x22, // constant, exprloc, reference
    DW_AT_producer                  = 0x25, // string
    DW_AT_prototyped                = 0x27, // flag
    DW_AT_return_addr               = 0x2a, // exprloc, loclist
    DW_AT_start_scope               = 0x2c, // constant, rnglist
    DW_AT_bit_stride                = 0x2e, // constant, exprloc, reference
    DW_AT_upper_bound               = 0x2f, // constant, exprloc, reference
    DW_AT_abstract_origin           = 0x31, // reference
    DW_AT_accessibility             = 0x32, // constant
    DW_AT_address_class             = 0x33, // constant
    DW_AT_artificial                = 0x34, // flag
    DW_AT_base_types                = 0x35, // reference
    DW_AT_calling_convention        = 0x36, // constant
    DW_AT_count                     = 0x37, // constant, exprloc, reference
    DW_AT_data_member_location      = 0x38, // constant, exprloc, loclist
    DW_AT_decl_column               = 0x39, // constant
    DW_AT_decl_file                 = 0x3a, // constant
    DW_AT_decl_line                 = 0x3b, // constant
    DW_AT_declaration               = 0x3c, // flag
    DW_AT_discr_list                = 0x3d, // block
    DW_AT_encoding                  = 0x3e, // constant
    DW_AT_external                  = 0x3f, // flag
    DW_AT_frame_base                = 0x40, // exprloc, loclist
    DW_AT_friend                    = 0x41, // reference
    DW_AT_identifier_case           = 0x42, // constantmacptr
    DW_AT_namelist_item             = 0x44, // reference
    DW_AT_priority                  = 0x45, // reference
    DW_AT_segment                   = 0x46, // exprloc, loclist
    DW_AT_specification             = 0x47, // reference
    DW_AT_static_link               = 0x48, // exprloc, loclist
    DW_AT_type                      = 0x49, // reference
    DW_AT_use_location              = 0x4a, // exprloc, loclist
    DW_AT_variable_parameter        = 0x4b, // flag
    DW_AT_virtuality                = 0x4c, // constant
    DW_AT_vtable_elem_location      = 0x4d, // exprloc, loclist
    DW_AT_allocated                 = 0x4e, // constant, exprloc, reference
    DW_AT_associated                = 0x4f, // constant, exprloc, reference
    DW_AT_data_location             = 0x50, // exprloc
    DW_AT_byte_stride               = 0x51, // constant, exprloc, reference
    DW_AT_entry_pc                  = 0x52, // address, constant
    DW_AT_use_UTF8                  = 0x53, // flag
    DW_AT_extension                 = 0x54, // reference
    DW_AT_ranges                    = 0x55, // rnglist
    DW_AT_trampoline                = 0x56, // address, flag, reference, string
    DW_AT_call_column               = 0x57, // constant
    DW_AT_call_file                 = 0x58, // constant
    DW_AT_call_line                 = 0x59, // constant
    DW_AT_description               = 0x5a, // string
    DW_AT_binary_scale              = 0x5b, // constant
    DW_AT_decimal_scale             = 0x5c, // constant
    DW_AT_small                     = 0x5d, // reference
    DW_AT_decimal_sign              = 0x5e, // constant
    DW_AT_digit_count               = 0x5f, // constant
    DW_AT_picture_string            = 0x60, // string
    DW_AT_mutable                   = 0x61, // flag
    DW_AT_threads_scaled            = 0x62, // flag
    DW_AT_explicit                  = 0x63, // flag
    DW_AT_object_pointer            = 0x64, // reference
    DW_AT_endianity                 = 0x65, // constant
    DW_AT_elemental                 = 0x66, // flag
    DW_AT_pure                      = 0x67, // flag
    DW_AT_recursive                 = 0x68, // flag
    DW_AT_signature                 = 0x69, // reference
    DW_AT_main_subprogram           = 0x6a, // flag
    DW_AT_data_bit_offset           = 0x6b, // constant
    DW_AT_const_expr                = 0x6c, // flag
    DW_AT_enum_class                = 0x6d, // flag
    DW_AT_linkage_name              = 0x6e, // string
    DW_AT_string_length_bit_size    = 0x6f, // constant
    DW_AT_string_length_byte_size   = 0x70, // constant
    DW_AT_rank                      = 0x71, // constant, exprloc
    DW_AT_str_offsets_base          = 0x72, // stroffsetsptr
    DW_AT_addr_base                 = 0x73, // addrptr
    DW_AT_rnglists_base             = 0x74, // rnglistsptr
    DW_AT_dwo_name                  = 0x76, // string
    DW_AT_reference                 = 0x77, // flag
    DW_AT_rvalue_reference          = 0x78, // flag
    DW_AT_macros                    = 0x79, // macptr
    DW_AT_call_all_calls            = 0x7a, // flag
    DW_AT_call_all_source_calls     = 0x7b, // flag
    DW_AT_call_all_tail_calls       = 0x7c, // flag
    DW_AT_call_return_pc            = 0x7d, // address
    DW_AT_call_value                = 0x7e, // exprloc
    DW_AT_call_origin               = 0x7f, // exprloc
    DW_AT_call_parameter            = 0x80, // reference
    DW_AT_call_pc                   = 0x81, // address
    DW_AT_call_tail_call            = 0x82, // flag
    DW_AT_call_target               = 0x83, // exprloc
    DW_AT_call_target_clobbered     = 0x84, // exprloc
    DW_AT_call_data_location        = 0x85, // exprloc
    DW_AT_call_data_value           = 0x86, // exprloc
    DW_AT_noreturn                  = 0x87, // flag
    DW_AT_alignment                 = 0x88, // constant
    DW_AT_export_symbols            = 0x89, // flag
    DW_AT_deleted                   = 0x8a, // flag
    DW_AT_defaulted                 = 0x8b, // constant
    DW_AT_loclists_base             = 0x8c, // loclistsptr
    // For Internal Use
    _Unknown,
    _Unused
};
enum {
    // Vendor Extensions
    DW_AT_lo_user                   = 0x2000,
    DW_AT_GNU_call_site_value       = 0x2111,
    DW_AT_GNU_call_site_target      = 0x2113,
    DW_AT_GNU_tail_call             = 0x2115,
    DW_AT_GNU_all_call_sites        = 0x2117,
    DW_AT_hi_user                   = 0x3fff,
};
// Table 7.6: Attribute form encodings
enum class _Form : std::uint8_t {
    _None,
    DW_FORM_addr                    = 0x01,
    DW_FORM_block2                  = 0x03,
    DW_FORM_block4                  = 0x04,
    DW_FORM_data2                   = 0x05,
    DW_FORM_data4                   = 0x06,
    DW_FORM_data8                   = 0x07,
    DW_FORM_string                  = 0x08,
    DW_FORM_block                   = 0x09,
    DW_FORM_block1                  = 0x0a,
    DW_FORM_data1                   = 0x0b,
    DW_FORM_flag                    = 0x0c,
    DW_FORM_sdata                   = 0x0d,
    DW_FORM_strp                    = 0x0e,
    DW_FORM_udata                   = 0x0f,
    DW_FORM_ref_addr                = 0x10,
    DW_FORM_ref1                    = 0x11,
    DW_FORM_ref2                    = 0x12,
    DW_FORM_ref4                    = 0x13,
    DW_FORM_ref8                    = 0x14,
    DW_FORM_ref_udata               = 0x15,
    DW_FORM_indirect                = 0x16,
    DW_FORM_sec_offset              = 0x17,
    DW_FORM_exprloc                 = 0x18,
    DW_FORM_flag_present            = 0x19,
    DW_FORM_strx                    = 0x1a,
    DW_FORM_addrx                   = 0x1b,
    DW_FORM_ref_sup4                = 0x1c,
    DW_FORM_strp_sup                = 0x1d,
    DW_FORM_data16                  = 0x1e,
    DW_FORM_line_strp               = 0x1f,
    DW_FORM_ref_sig8                = 0x20,
    DW_FORM_implicit_const          = 0x21,
    DW_FORM_loclistx                = 0x22,
    DW_FORM_rnglistx                = 0x23,
    DW_FORM_ref_sup8                = 0x24,
    DW_FORM_strx1                   = 0x25,
    DW_FORM_strx2                   = 0x26,
    DW_FORM_strx3                   = 0x27,
    DW_FORM_strx4                   = 0x28,
    DW_FORM_addrx1                  = 0x29,
    DW_FORM_addrx2                  = 0x2a,
    DW_FORM_addrx3                  = 0x2b,
    DW_FORM_addrx4                  = 0x2c,
    // For Internal Use
    _HighPC,
    _DeclFile,
    _Unused
};
// Table 7.9: DWARF operation encodings
enum {
    _None,
    DW_OP_addr                      = 0x03,
    DW_OP_deref                     = 0x06,
    DW_OP_const1u                   = 0x08,
    DW_OP_const1s                   = 0x09,
    DW_OP_const2u                   = 0x0a,
    DW_OP_const2s                   = 0x0b,
    DW_OP_const4u                   = 0x0c,
    DW_OP_const4s                   = 0x0d,
    DW_OP_const8u                   = 0x0e,
    DW_OP_const8s                   = 0x0f,
    DW_OP_constu                    = 0x10,
    DW_OP_consts                    = 0x11,
    DW_OP_dup                       = 0x12,
    DW_OP_drop                      = 0x13,
    DW_OP_over                      = 0x14,
    DW_OP_pick                      = 0x15,
    DW_OP_swap                      = 0x16,
    DW_OP_rot                       = 0x17,
    DW_OP_xderef                    = 0x18,
    DW_OP_abs                       = 0x19,
    DW_OP_and                       = 0x1a,
    DW_OP_div                       = 0x1b,
    DW_OP_minus                     = 0x1c,
    DW_OP_mod                       = 0x1d,
    DW_OP_mul                       = 0x1e,
    DW_OP_neg                       = 0x1f,
    DW_OP_not                       = 0x20,
    DW_OP_or                        = 0x21,
    DW_OP_plus                      = 0x22,
    DW_OP_plus_uconst               = 0x23,
    DW_OP_shl                       = 0x24,
    DW_OP_shr                       = 0x25,
    DW_OP_shra                      = 0x26,
    DW_OP_xor                       = 0x27,
    DW_OP_bra                       = 0x28,
    DW_OP_eq                        = 0x29,
    DW_OP_ge                        = 0x2a,
    DW_OP_gt                        = 0x2b,
    DW_OP_le                        = 0x2c,
    DW_OP_lt                        = 0x2d,
    DW_OP_ne                        = 0x2e,
    DW_OP_skip                      = 0x2f,
    DW_OP_lit0                      = 0x30,
    DW_OP_lit1                      = 0x31,
    DW_OP_lit2                      = 0x32,
    DW_OP_lit3                      = 0x33,
    DW_OP_lit4                      = 0x34,
    DW_OP_lit5                      = 0x35,
    DW_OP_lit6                      = 0x36,
    DW_OP_lit7                      = 0x37,
    DW_OP_lit8                      = 0x38,
    DW_OP_lit9                      = 0x39,
    DW_OP_lit10                     = 0x3a,
    DW_OP_lit11                     = 0x3b,
    DW_OP_lit12                     = 0x3c,
    DW_OP_lit13                     = 0x3d,
    DW_OP_lit14                     = 0x3e,
    DW_OP_lit15                     = 0x3f,
    DW_OP_lit16                     = 0x40,
    DW_OP_lit17                     = 0x41,
    DW_OP_lit18                     = 0x42,
    DW_OP_lit19                     = 0x43,
    DW_OP_lit20                     = 0x44,
    DW_OP_lit21                     = 0x45,
    DW_OP_lit22                     = 0x46,
    DW_OP_lit23                     = 0x47,
    DW_OP_lit24                     = 0x48,
    DW_OP_lit25                     = 0x49,
    DW_OP_lit26                     = 0x4a,
    DW_OP_lit27                     = 0x4b,
    DW_OP_lit28                     = 0x4c,
    DW_OP_lit29                     = 0x4d,
    DW_OP_lit30                     = 0x4e,
    DW_OP_lit31                     = 0x4f,
    DW_OP_reg0                      = 0x50,
    DW_OP_reg1                      = 0x51,
    DW_OP_reg2                      = 0x52,
    DW_OP_reg3                      = 0x53,
    DW_OP_reg4                      = 0x54,
    DW_OP_reg5                      = 0x55,
    DW_OP_reg6                      = 0x56,
    DW_OP_reg7                      = 0x57,
    DW_OP_reg8                      = 0x58,
    DW_OP_reg9                      = 0x59,
    DW_OP_reg10                     = 0x5a,
    DW_OP_reg11                     = 0x5b,
    DW_OP_reg12                     = 0x5c,
    DW_OP_reg13                     = 0x5d,
    DW_OP_reg14                     = 0x5e,
    DW_OP_reg15                     = 0x5f,
    DW_OP_reg16                     = 0x60,
    DW_OP_reg17                     = 0x61,
    DW_OP_reg18                     = 0x62,
    DW_OP_reg19                     = 0x63,
    DW_OP_reg20                     = 0x64,
    DW_OP_reg21                     = 0x65,
    DW_OP_reg22                     = 0x66,
    DW_OP_reg23                     = 0x67,
    DW_OP_reg24                     = 0x68,
    DW_OP_reg25                     = 0x69,
    DW_OP_reg26                     = 0x6a,
    DW_OP_reg27                     = 0x6b,
    DW_OP_reg28                     = 0x6c,
    DW_OP_reg29                     = 0x6d,
    DW_OP_reg30                     = 0x6e,
    DW_OP_reg31                     = 0x6f,
    DW_OP_breg0                     = 0x70,
    DW_OP_breg1                     = 0x71,
    DW_OP_breg2                     = 0x72,
    DW_OP_breg3                     = 0x73,
    DW_OP_breg4                     = 0x74,
    DW_OP_breg5                     = 0x75,
    DW_OP_breg6                     = 0x76,
    DW_OP_breg7                     = 0x77,
    DW_OP_breg8                     = 0x78,
    DW_OP_breg9                     = 0x79,
    DW_OP_breg10                    = 0x7a,
    DW_OP_breg11                    = 0x7b,
    DW_OP_breg12                    = 0x7c,
    DW_OP_breg13                    = 0x7d,
    DW_OP_breg14                    = 0x7e,
    DW_OP_breg15                    = 0x7f,
    DW_OP_breg16                    = 0x80,
    DW_OP_breg17                    = 0x81,
    DW_OP_breg18                    = 0x82,
    DW_OP_breg19                    = 0x83,
    DW_OP_breg20                    = 0x84,
    DW_OP_breg21                    = 0x85,
    DW_OP_breg22                    = 0x86,
    DW_OP_breg23                    = 0x87,
    DW_OP_breg24                    = 0x88,
    DW_OP_breg25                    = 0x89,
    DW_OP_breg26                    = 0x8a,
    DW_OP_breg27                    = 0x8b,
    DW_OP_breg28                    = 0x8c,
    DW_OP_breg29                    = 0x8d,
    DW_OP_breg30                    = 0x8e,
    DW_OP_breg31                    = 0x8f,
    DW_OP_regx                      = 0x90,
    DW_OP_fbreg                     = 0x91,
    DW_OP_bregx                     = 0x92,
    DW_OP_piece                     = 0x93,
    DW_OP_deref_size                = 0x94,
    DW_OP_nop                       = 0x96,
    DW_OP_push_object_address       = 0x97,
    DW_OP_call2                     = 0x98,
    DW_OP_call4                     = 0x99,
    DW_OP_call_ref                  = 0x9a,
    DW_OP_form_tls_address          = 0x9b,
    DW_OP_call_fram_cfa             = 0x9c,
    DW_OP_bit_piece                 = 0x9d,
    DW_OP_implicit_value            = 0x9e,
    DW_OP_stack_value               = 0x9f,
    DW_OP_implicit_pointer          = 0xa0,
    DW_OP_addrx                     = 0xa1,
    DW_OP_constx                    = 0xa2,
    DW_OP_entry_value               = 0xa3,
    DW_OP_const_type                = 0xa4,
    DW_OP_regval_type               = 0xa5,
    DW_OP_deref_type                = 0xa6,
    DW_OP_xderef_type               = 0xa7,
    DW_OP_convert                   = 0xa8,
    DW_OP_reinterpret               = 0xa9,
    // Vendor Extensions
    DW_OP_lo_user                   = 0xe0,
    DW_OP_GNU_entry_value           = 0xf3,
    DW_OP_hi_user                   = 0xff,
};

class File;

namespace section {
struct _Die;
class Line;
class LocLists;
class RngLists;

using _Rng = std::pair<_Addr, _Addr>;
using _RngView = _View<const _Rng>;

struct _Loc {
    _Rng _M_range;
    Data _M_expr;
};
using _LocView = _View<const _Loc>;

class SrcFile {
    using _Lines = std::map<std::pair<_Word, _Word>, _Addr>;

public:
    static const SrcFile _S_null;

    SrcFile() noexcept : _M_md5(nullptr) {}
    explicit SrcFile(const _Path<3> &__path, Data::element_type *__md5) noexcept
        : _M_path(__path), _M_md5(__md5) {}
    constexpr const _Path<3> &path()      const noexcept { return _M_path;  }
    constexpr       _Lines   &lines()           noexcept { return _M_lines; }
    constexpr const _Lines   &lines()     const noexcept { return _M_lines; }
    constexpr Data::element_type *md5() const noexcept { return _M_md5;   }

private:
    _Path<3> _M_path;
    Data::element_type *_M_md5;
    _Lines _M_lines;
};

class _Val {
    enum class _Kind : std::uint8_t {
        _None,
        _Addr,
        _Cst,
        _Data,
        _File,
        _Flag,
        _Ref,
        _LocList,
        _RngList,
        _Str,
    };

    static constexpr bool is_large(std::size_t bits) noexcept {
        return bits > std::numeric_limits<std::uintptr_t>::digits;
    }
    constexpr std::size_t size() const noexcept { return _M_addr; }
    constexpr bool is_large() const noexcept {
        return is_cst() && size() > std::numeric_limits<decltype(_M_small)>::digits;
    }
    std::uintmax_t *take_large() const & { return new std::uintmax_t(*_M_large); }
    std::uintmax_t *take_large() && { return std::exchange(_M_large, nullptr); }
    void maybe_moved_from() const & {}
    void maybe_moved_from() && {
        _M_kind = _Kind::_None;
        _U_addr = {};
        _U_ptr = {};
    }
    template<typename _Ty> _Val &copy(_Ty &&__val) {
        if (this == &__val) return *this;
        if (is_large() && __val.is_large()) {
            _M_flag = __val._M_flag;
            _M_addr = __val._M_addr;
            *_M_large = *__val._M_large;
            return *this;
        }
        this->~_Val();
        _M_kind = __val._M_kind;
        _M_flag = __val._M_flag;
        switch (_M_kind) {
            case _Kind::_None:
            case _Kind::_File:
            case _Kind::_Flag:
            case _Kind::_Ref:
                _U_addr = __val._U_addr;
                break;
            case _Kind::_Addr:
            case _Kind::_Cst:
            case _Kind::_Data:
            case _Kind::_LocList:
            case _Kind::_RngList:
            case _Kind::_Str:
                _M_addr = __val._M_addr;
                break;
        }
        switch (_M_kind) {
            case _Kind::_None:
            case _Kind::_Addr:
            case _Kind::_Flag:
                _U_ptr = __val._U_ptr;
                break;
            case _Kind::_Cst:
                if (is_large())
                    _M_large = std::forward<_Ty>(__val).take_large();
                else
                    _M_small = __val._M_small;
                break;
            case _Kind::_Data:
                _M_data = __val._M_data;
                break;
            case _Kind::_File:
                _M_file = __val._M_file;
                break;
            case _Kind::_Ref:
                _M_ref = __val._M_ref;
                break;
            case _Kind::_LocList:
                _M_loclist = __val._M_loclist;
                break;
            case _Kind::_RngList:
                _M_rnglist = __val._M_rnglist;
                break;
            case _Kind::_Str:
                _M_str = __val._M_str;
                break;
        }
        std::forward<_Ty>(__val).maybe_moved_from();
        return *this;
    }

public:
    static const _Val _S_none;

    _Val(const _Val &__val) : _Val() { copy(__val); }
    _Val(_Val &&__val) : _Val() { copy(std::move(__val)); }
    _Val &operator=(const _Val &__val) { return copy(__val); }
    _Val &operator=(_Val &&__val) { return copy(std::move(__val)); }
    ~_Val() noexcept { if (is_large()) delete _M_large; }

    constexpr _Val() noexcept : _M_kind(_Kind::_None), _M_flag(), _U_addr(), _U_ptr() {}
    constexpr bool is_none() const noexcept { return _M_kind == _Kind::_None; }

    constexpr _Val(_Addr __addr) noexcept : _M_kind(_Kind::_Addr), _M_flag(), _M_addr(__addr), _U_ptr() {}
    constexpr bool is_addr() const noexcept { return _M_kind == _Kind::_Addr; }
    constexpr _Addr addr() const noexcept { if (is_addr()) return _M_addr; return {}; }

    template<typename _Int> _Val(detail::SizedInteger<_Int> __cst)
        : _M_kind(_Kind::_Cst), _M_flag(std::is_signed<_Int>::value), _M_addr(__cst.width()), _U_ptr() {
        if (is_large()) {
            _M_large = new std::uintmax_t(__cst.get());
        } else {
            _M_small = __cst.get();
        }
    }
    template<typename _Int, std::enable_if_t<std::numeric_limits<_Int>::is_integer, int> = 0>
    _Val(_Int __cst) : _Val(detail::SizedInteger<_Int>(__cst)) {}
    constexpr bool is_cst() const noexcept { return _M_kind == _Kind::_Cst; }
    constexpr std::uintmax_t cst() const noexcept {
        if (is_large()) return *_M_large;
        if (is_cst()) return _M_small;
        if (is_data()) return data().partial<std::uintmax_t>();
        return {};
    }

    constexpr _Val(Data __data) noexcept
        : _M_kind(_Kind::_Data), _M_flag(), _M_addr(__data.size()),
          _M_data(__data.data()) { assert(size() == __data.size() && "Overflow"); }
    constexpr bool is_data() const noexcept { return _M_kind == _Kind::_Data; }
    constexpr Data data() const noexcept { if (is_data()) return { _M_data, size() }; return {}; }

    constexpr _Val(const _Die &__ref) noexcept
        : _M_kind(_Kind::_Ref), _M_flag(), _U_addr(), _M_ref(&__ref) {}
    constexpr bool is_ref() const noexcept { return _M_kind == _Kind::_Ref; }
    constexpr const _Die &ref() const noexcept;

    constexpr _Val(const SrcFile &__file)
        : _M_kind(_Kind::_File), _M_flag(), _U_addr(), _M_file(&__file) {}
    constexpr bool is_file() const noexcept { return _M_kind == _Kind::_File; }
    constexpr const SrcFile &file() const noexcept { return is_file() ? *_M_file : SrcFile::_S_null; }

    constexpr _Val(bool __flag) noexcept
        : _M_kind(_Kind::_Flag), _M_flag(__flag), _U_addr(), _U_ptr() {}
    constexpr bool is_flag() const noexcept { return _M_kind == _Kind::_Flag; }
    constexpr bool flag() const noexcept { if (is_flag()) return _M_flag; return {}; }

    constexpr _Val(_LocView __loclist) noexcept
        : _M_kind(_Kind::_LocList), _M_flag(), _M_addr(__loclist.size()),
          _M_loclist(__loclist.data()) { assert(size() == __loclist.size() && "Overflow"); }
    constexpr bool is_loclist() const noexcept { return _M_kind == _Kind::_LocList; }
    constexpr _LocView loclist() const noexcept {
        if (is_loclist()) return { _M_loclist, size() };
        return {};
    }

    constexpr _Val(_RngView __rnglist) noexcept
        : _M_kind(_Kind::_RngList), _M_flag(), _M_addr(__rnglist.size()),
          _M_rnglist(__rnglist.data()) { assert(size() == __rnglist.size() && "Overflow"); }
    constexpr bool is_rnglist() const noexcept { return _M_kind == _Kind::_RngList; }
    constexpr _RngView rnglist() const noexcept {
        if (is_rnglist()) return { _M_rnglist, size() };
        return {};
    }

    constexpr _Val(_Str __str) noexcept
        : _M_kind(_Kind::_Str), _M_flag(), _M_addr(__str.size()),
          _M_str(__str.data()) { assert(size() == __str.size() && "Overflow"); }
    constexpr bool is_str() const noexcept { return _M_kind == _Kind::_Str; }
    constexpr _Str str() const noexcept { if (is_str()) return { _M_str, size() }; return {}; }

private:
    _Kind _M_kind : 4;
    bool  _M_flag : 1;
    union {
        struct {} _U_addr;
        _Addr _M_addr;
    };
    union {
        struct {} _U_ptr;
        std::uintptr_t _M_small;
        std::uintmax_t *_M_large;
        Data::pointer _M_data;
        const SrcFile *_M_file;
        const _Die *_M_ref;
        _LocView::pointer _M_loclist;
        _RngView::pointer _M_rnglist;
        _Str::pointer _M_str;
    };
};

struct _Die {
    _Tag _M_tag : std::numeric_limits<_Sbyte>::digits;
    bool _M_children : 1;
    std::uint32_t _M_sibling : std::numeric_limits<_Word>::digits -
                               std::numeric_limits<_Byte>::digits;
    std::unordered_map<_At, _Val> _M_attrs;

    const _Val &attr(_At __at) const noexcept;

    static const _Die _S_null;
};

constexpr const _Die &_Val::ref() const noexcept { return is_ref() ? *_M_ref : _Die::_S_null; }

class Base {
public:
    constexpr Base(File &__file) noexcept : _M_file(__file) {}
    constexpr void error(const char *__error) const noexcept;
    constexpr const char *error() const noexcept;
    constexpr void data(Data __data) noexcept { _M_data = __data; }

    File &_M_file;
    Data _M_data;
};

class Abbrev : public Base {
public:
    struct _Attr {
        _At _M_at;
        _Form _M_form;
        std::int64_t _M_implicit_const;
    };
    struct _Entry {
        _Tag _M_tag : std::numeric_limits<std::int8_t>::digits;
        bool _M_children : 1;
        std::vector<_Attr> _M_attrs;
    };

    Abbrev(File &__file) noexcept : Base(__file) {}
    std::unordered_map<std::uint32_t, _Entry> get(std::uint32_t __base) const noexcept;
};

class Addr : public Base {
public:
    Addr(File &__file) noexcept : Base(__file) {}
    _AddrView get(const _Die &__unit_entry) const noexcept;
};

class Frame : public Base {
public:
    class Rule {
        enum class _Kind : std::uint8_t {
            _Undef,
            _SameVal,
            _Off,
            _ValOff,
            _Reg,
            _RegOff,
            _Expr,
            _ValExpr,
            _Arch,
        };

        constexpr Rule(_Kind __kind, Reg __reg = Reg::None) noexcept : Rule(__kind, __reg, {}) {}
        constexpr Rule(_Kind __kind, _Addr __off) noexcept
            : _M_kind(__kind), _M_reg(Reg::None), _M_off(__off) {}
        constexpr Rule(_Kind __kind, Reg __reg, _Addr __off) noexcept
            : _M_kind(__kind), _M_reg(__reg), _M_off(__off) {}

    public:
        constexpr Rule() noexcept : Rule(_Kind::_Undef) {}
        static constexpr Rule undef() noexcept { return {}; }
        constexpr bool is_undef() const noexcept { return _M_kind == _Kind::_Undef; }

        static constexpr Rule same_val() noexcept { return { _Kind::_SameVal }; }
        constexpr bool is_same_val() const noexcept { return _M_kind == _Kind::_SameVal; }

        static constexpr Rule off(std::int32_t __off) noexcept { return { _Kind::_Off, std::uint32_t(__off) }; }
        constexpr bool is_off() const noexcept { return _M_kind == _Kind::_Off; }

        static constexpr Rule val_off(std::int32_t __off) noexcept { return { _Kind::_ValOff, std::uint32_t(__off) }; }
        constexpr bool is_val_off() const noexcept { return _M_kind == _Kind::_ValOff; }

        static constexpr Rule reg(Reg __reg) noexcept { return { _Kind::_ValOff, __reg }; }
        constexpr bool is_reg() const noexcept { return _M_kind == _Kind::_Reg; }

        static constexpr Rule reg_off(Reg __reg, std::int32_t __off) noexcept {
            return { _Kind::_RegOff, __reg, std::uint32_t(__off) };
        }
        constexpr bool is_reg_off() const noexcept { return _M_kind == _Kind::_RegOff; }

        static Rule expr(Frame &__frame, Data __expr) noexcept {
            return { _Kind::_Expr, __frame.expr(__expr) };
        }
        constexpr bool is_expr() const noexcept { return _M_kind == _Kind::_Expr; }

        static Rule val_expr(Frame &__frame, Data __expr) noexcept {
            return { _Kind::_ValExpr, __frame.expr(__expr) };
        }
        constexpr bool is_val_expr() const noexcept { return _M_kind == _Kind::_ValExpr; }

        constexpr bool is_arch() const noexcept { return _M_kind == _Kind::_Arch; }

        constexpr bool is_val() const noexcept {
            return is_same_val() || is_reg() || is_reg_off() || is_val_off() || is_val_expr();
        }
        constexpr bool is_addr() const noexcept { return is_off() || is_expr(); }

        constexpr Reg reg() const noexcept { return _M_reg; }
        constexpr std::int32_t off() const noexcept { return _M_off; }
        constexpr Data expr(const Frame &__frame) const noexcept {
            if (is_expr() || is_val_expr())
                return __frame.expr(_M_off);
            return {};
        }

    private:
        _Kind _M_kind : 4;
        Reg _M_reg : 4;
        _Addr _M_off;
    };
    class RuleSet {
    public:
        constexpr Rule &cfa() noexcept { return _M_cfa; }
        constexpr Rule cfa() const noexcept { return _M_cfa; }
        constexpr Rule &operator[](Reg __reg) noexcept {
            assert(__reg >= Reg() && __reg < Reg::None);
            return _M_rules[std::size_t(__reg)];
        }
        constexpr Rule operator[](Reg __reg) const noexcept {
            if (__reg >= Reg() && __reg < Reg::None)
                return _M_rules[std::size_t(__reg)];
            return {};
        }

        static constexpr const RuleSet &undef() noexcept { return _S_undef; }

    private:
        Rule _M_cfa;
        Rule _M_rules[std::size_t(Reg::None)];

        static const RuleSet _S_undef;
    };

private:
    using _Exprs = std::vector<Data>;
    using _Rules = std::map<_Addr, RuleSet>;

    friend Rule;
    _Exprs::size_type expr(Data __expr) noexcept {
        auto __pos = _M_exprs.size();
        _M_exprs.push_back(__expr);
        return __pos;
    }
    Data expr(_Exprs::size_type __pos) const noexcept {
        if (__pos < _M_exprs.size())
            return _M_exprs[__pos];
        return {};
    }

public:
    Frame(File &__file) noexcept : Base(__file) {}
    void __parse();
    const RuleSet &get(_Addr __addr) const noexcept;

private:
    _Exprs _M_exprs;
    _Rules _M_rules;
};

class Info : public Base {
    // Table 7.2: Unit header unit type encodings
    enum {
        DW_UT_compile       = 0x01,
        DW_UT_type          = 0x02,
        DW_UT_partial       = 0x03,
        DW_UT_skeleton      = 0x04,
        DW_UT_split_compile = 0x05,
        DW_UT_split_type    = 0x06,
    };

public:
    class _Scope {
    public:
        _Scope(const _Scope &__parent = _S_null, const _Die &__entry = _Die::_S_null) noexcept
            : _M_parent(__parent), _M_entry(__entry) {}
        void set(_Addr __low, _Addr __high, _Scope &__child) {
            _M_children.insert({__low, { __high, __child } });
        }
        void add(const _Die &__entry) { _M_entities.push_back(__entry); }

        explicit constexpr operator bool() const noexcept { return this != &_S_null; }
        static constexpr const _Scope &null() noexcept { return _S_null; }

        constexpr const _Scope &parent() const noexcept { return _M_parent; }
        constexpr bool is_unit() const noexcept {
            return _M_entry._M_tag == _Tag::DW_TAG_compile_unit;
        }
        constexpr bool is_function() const noexcept {
            return _M_entry._M_tag == _Tag::DW_TAG_inlined_subroutine ||
                   _M_entry._M_tag == _Tag::DW_TAG_subprogram;
        }
        constexpr const _Scope &function() const noexcept {
            return is_function() ? *this : _M_parent ? _M_parent.function() : null();
        }
        constexpr const _Die &entry() const noexcept { return _M_entry; }
        auto begin() const { return _M_entities.begin(); }
        auto end()   const { return _M_entities.end();   }
        const _Scope &get(_Addr __addr) const noexcept;
        void dump[[gnu::used]](int indent = 0) const;

    private:
        const _Scope &_M_parent;
        const _Die &_M_entry;
        std::vector<std::reference_wrapper<const _Die>> _M_entities;
        std::map<_Addr, std::pair<_Addr, const _Scope &>> _M_children;

        static const _Scope _S_null;
    };

    Info(File &__file) noexcept : Base(__file), _M_scopes{{}} {}
    void __parse(Line &line, LocLists &loclists, RngLists &rnglists);
    const _Scope &root() const noexcept { return _M_scopes.front(); }

private:
    std::vector<_Die> _M_entries;
    std::deque<_Scope> _M_scopes;
};

class Line : public Base {
    // Table 7.25: Line number standard opcode encodings
    enum {
        DW_LNS_extended           = 0x00,
        DW_LNS_copy               = 0x01,
        DW_LNS_advance_pc         = 0x02,
        DW_LNS_advance_line       = 0x03,
        DW_LNS_set_file           = 0x04,
        DW_LNS_set_column         = 0x05,
        DW_LNS_negate_stmt        = 0x06,
        DW_LNS_set_basic_block    = 0x07,
        DW_LNS_const_add_pc       = 0x08,
        DW_LNS_fixed_advance_pc   = 0x09,
        DW_LNS_set_prologue_end   = 0x0a,
        DW_LNS_set_epilogue_begin = 0x0b,
        DW_LNS_set_isa            = 0x0c,
    };
    // Table 7.26: Line number extended opcode encodings
    enum {
        DW_LNE_end_sequence       = 0x01,
        DW_LNE_set_address        = 0x02,
        DW_LNE_set_discriminator  = 0x04,
        // Vendor Extensions
        DW_LNE_lo_user            = 0x80,
        DW_LNE_hi_user            = 0xff,
    };
    // Table 7.27: Line number header entry format encodings
    enum class _Lnct : std::uint8_t {
        DW_LNCT_path              = 0x1,
        DW_LNCT_directory_index   = 0x2,
        DW_LNCT_timestamp         = 0x3,
        DW_LNCT_size              = 0x4,
        DW_LNCT_MD5               = 0x5,
    };
    enum {
        // Vendor Extensions
        DW_LNCT_lo_user           = 0x2000,
        DW_LNCT_hi_user           = 0x3fff,
    };

    using _EntryFormat = std::pair<_Lnct, _Form>;
    struct _Entry {
        _Str _M_path;
        _Word _M_directory_index = _InvalidWord;
        _Long _M_timestamp{};
        _Long _M_size{};
        Data _M_MD5;
    };

public:
    struct _Loc {
        std::uint32_t _M_file;
        std::uint32_t _M_line;
        std::uint32_t _M_column;
        std::uint32_t _M_address        : 24;
        bool          _M_is_stmt        :  1;
        bool          _M_basic_block    :  1;
        bool          _M_end_sequence   :  1;
        bool          _M_prologue_end   :  1;
        bool          _M_epilogue_begin :  1;
        std::uint8_t  _M_isa            :  1;
        std::uint8_t  _M_discriminator  :  2;
    };

private:
    using _Files = std::vector<SrcFile>;
    using _Paths = std::unordered_map<_Path<3>, std::uint32_t>;
    using _Locs = std::map<std::uint32_t, _Loc>;

public:
    Line(File &__file) noexcept : Base(__file) {}
    std::vector<std::uint32_t> __parse(std::uint8_t __address_size, const _Die &__unit_entry);
    void shrink_to_fit() noexcept { _M_files.shrink_to_fit(); }
    const SrcFile &get(_Files::size_type __pos) const noexcept;
    _Files::size_type size() const noexcept { return _M_files.size(); }
    constexpr const _Locs &locs() const noexcept { return _M_locs; }

private:
    _Files _M_files;
    _Paths _M_paths;
    _Locs _M_locs;
};

class LocLists : public Base {
    // Table 7.10: Location list entry encoding values
    enum {
        DW_LLE_end_of_list      = 0x00,
        DW_LLE_base_addressx    = 0x01,
        DW_LLE_startx_endx      = 0x02,
        DW_LLE_startx_length    = 0x03,
        DW_LLE_offset_pair      = 0x04,
        DW_LLE_default_location = 0x05,
        DW_LLE_base_address     = 0x06,
        DW_LLE_start_end        = 0x07,
        DW_LLE_start_length     = 0x08,
    };

public:
    LocLists(File &__file) noexcept : Base(__file) {}
    std::vector<std::pair<std::uint32_t, std::uint32_t>> __parse(const _Die &__unit_entry);
    void shrink_to_fit() noexcept { _M_locs.shrink_to_fit(); }
    _LocView get(std::pair<std::uint32_t, std::uint32_t> __indices) const noexcept;

private:
    std::vector<_Loc> _M_locs;
};

class RngLists : public Base {
    // Table 7.30: Range list entry encoding values
    enum {
        DW_RLE_end_of_list   = 0x00,
        DW_RLE_base_addressx = 0x01,
        DW_RLE_startx_endx   = 0x02,
        DW_RLE_startx_length = 0x03,
        DW_RLE_offset_pair   = 0x04,
        DW_RLE_base_address  = 0x05,
        DW_RLE_start_end     = 0x06,
        DW_RLE_start_length  = 0x07,
    };

public:
    RngLists(File &__file) noexcept : Base(__file) {}
    std::vector<std::pair<std::uint32_t, std::uint32_t>> __parse(const _Die &__unit_entry);
    void shrink_to_fit() noexcept { _M_rngs.shrink_to_fit(); }
    _RngView get(std::pair<std::uint32_t, std::uint32_t> __indices) const noexcept;

private:
    std::vector<_Rng> _M_rngs;
};

class Str : public Base {
public:
    Str(File &__file) noexcept : Base(__file) {}
    _Str get(std::uint32_t __offset) const noexcept;
};

class StrOffsets : public Base {
public:
    StrOffsets(File &__file) noexcept : Base(__file) {}
    _OffView get(const _Die &__unit_entry) const noexcept;
};
}

class File : public elf::File {
public:
    explicit File(Data __data)
        : elf::File(__data), _M_error(nullptr), _M_abbrev(*this),
          _M_addr(*this), _M_frame(*this), _M_info(*this), _M_line(*this),
          _M_line_str(*this), _M_line_str_offsets(*this), _M_loclists(*this),
          _M_rnglists(*this), _M_str(*this), _M_str_offsets(*this) {}

    void validate(const Header &__expected);

    constexpr void error(const char *__error) const noexcept {
        if (!_M_error) {
            _M_error = __error;
        }
    }
    constexpr const char *error() const noexcept { return _M_error; };

    constexpr const section::Abbrev &abbrev() const noexcept { return _M_abbrev; }
    constexpr const section::Addr &addr() const noexcept { return _M_addr; }
    constexpr const section::Frame &frame() const noexcept { return _M_frame; }
    constexpr const section::Info &info() const noexcept { return _M_info; }
    constexpr const section::Line &line() const noexcept { return _M_line; }
    constexpr const section::Str &line_str() const noexcept { return _M_line_str; }
    constexpr const section::StrOffsets &line_str_offsets() const noexcept { return _M_line_str_offsets; }
    constexpr const section::LocLists &loclists() const noexcept { return _M_loclists; }
    constexpr const section::RngLists &rnglists() const noexcept { return _M_rnglists; }
    constexpr const section::Str &str() const noexcept { return _M_str; }
    constexpr const section::StrOffsets &str_offsets() const noexcept { return _M_str_offsets; }

private:
    mutable const char *_M_error;
    section::Abbrev _M_abbrev;
    section::Addr _M_addr;
    section::Frame _M_frame;
    section::Info _M_info;
    section::Line _M_line;
    section::Str _M_line_str;
    section::StrOffsets _M_line_str_offsets;
    section::LocLists _M_loclists;
    section::RngLists _M_rnglists;
    section::Str _M_str;
    section::StrOffsets _M_str_offsets;
};

constexpr void section::Base::error(const char *__error) const noexcept { _M_file.error(__error); }
constexpr const char *section::Base::error() const noexcept { return _M_file.error(); }
} // end namespace elf
} // end namespace debuginfo

#endif
