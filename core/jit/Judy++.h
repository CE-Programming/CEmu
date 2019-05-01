#ifndef JUDYPP_H
#define JUDYPP_H

#include "optional.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <utility>

namespace Judy {

using Word = unsigned long;
using Size = std::size_t;

template<typename Key = Word, typename Value = Word> class Judy;

class Judy1 {
public:
    bool Test(Word index) const;
    bool Set(Word index);
    void SetArray(const Word *indices, Size count);
    bool Unset(Word index);
    Size Count(Word first = Word(), Word last = ~Word()) const;
    tl::optional<Word> ByCount(Word count) const;
    Size FreeArray();
    Size MemUsed() const;
    Size MemActive() const;
    tl::optional<Word> First(Word index = Word()) const;
    tl::optional<Word> Next(Word index) const;
    tl::optional<Word> Last(Word index = ~Word()) const;
    tl::optional<Word> Prev(Word index) const;
    tl::optional<Word> FirstEmpty(Word index) const;
    tl::optional<Word> NextEmpty(Word index) const;
    tl::optional<Word> LastEmpty(Word index) const;
    tl::optional<Word> PrevEmpty(Word index) const;

    ~Judy1() noexcept { FreeArray(); }
    template<typename C>
    void SetArray(const C &c) { SetArray(c.cbegin(), c.size()); }
    bool Empty() { return !array; }

private:
    void *array = nullptr;
};

class JudyL {
public:
    using Entry = std::pair<Word, Word *>;

    Word *Get(Word index);
    const Word *Get(Word index) const;
    Word *Ins(Word index);
    void InsArray(const Word *indices, const Word *values, Size count);
    bool Del(Word index);
    Size Count(Word first = Word(), Word last = ~Word()) const;
    tl::optional<Entry> ByCount(Word count) const;
    Size FreeArray();
    Size MemUsed() const;
    Size MemActive() const;
    tl::optional<Entry> First(Word index = Word()) const;
    tl::optional<Entry> Next(Word index) const;
    tl::optional<Entry> Last(Word index = ~Word()) const;
    tl::optional<Entry> Prev(Word index) const;
    tl::optional<Word> FirstEmpty(Word index = Word()) const;
    tl::optional<Word> NextEmpty(Word index) const;
    tl::optional<Word> LastEmpty(Word index = ~Word()) const;
    tl::optional<Word> PrevEmpty(Word index) const;

    ~JudyL() { FreeArray(); }
    bool invalid() const noexcept;
    void *user() const noexcept;
    bool Empty() { return !array; }

private:
    void *array = nullptr;
};

class Util {
    Util() = delete;

    template<typename Key, typename Value> friend class Judy;
    template<typename Type> static Type to(Word index) {
        static_assert(std::is_trivial<Type>::value, "Type must be trivial");
        static_assert(sizeof(Type) <= sizeof(Word), "Type can't be bigger than Word");
        Type other;
        std::memcpy(&other, &index, sizeof(Type));
        return other;
    }
    template<typename Type> static tl::optional<Type> to(const tl::optional<Word> &other) {
        if (other)
            return to<Type>(*other);
        return tl::nullopt;
    }
    template<typename Type> static tl::optional<Type> to(const tl::optional<JudyL::Entry> &other) {
        if (other)
            return std::make_pair(to<typename Type::first_type>(other->first),
                                  std::ref(*reinterpret_cast<typename Type::second_type::type *>(other->second)));
        return tl::nullopt;
    }
    template<typename Type> static Word from(const Type &other) {
        static_assert(std::is_trivially_copyable<Type>::value, "Type must be trivially copyable");
        static_assert(sizeof(Type) <= sizeof(Word), "Type can't be bigger than Word");
        Word index{};
        std::memcpy(&index, &other, sizeof(Type));
        return index;
    }
    template<typename Type> static tl::optional<Word> from(const tl::optional<Type> &other) {
        return other ? from(*other) : tl::nullopt;
    }

    class Ref1 {
        template<typename Key, typename Value> friend class Judy;
        Judy1 &judy;
        Word index;
        Ref1(Judy1 &judy, Word index) : judy(judy), index(index) {}
    public:
        Ref1 &operator=(bool value) {
            (judy.*(value ? &Judy1::Set : &Judy1::Unset))(index);
            return *this;
        }
    };
};

template<typename Key, typename Value>
class Judy : JudyL {
    static_assert(sizeof(Key) <= sizeof(Word), "Key type must fit in pointer-width storage");
    static_assert(std::is_trivial<Key>::value, "Key must be trivial");
    static_assert(sizeof(Value) <= sizeof(Word), "Value type must fit in pointer-width storage");
    static_assert(std::is_trivial<Value>::value, "Value must be trivial");
    using Entry = std::pair<Key, std::reference_wrapper<Value>>;
public:
    using key_type = Key;
    using value_type = Value;
    Value &operator[](Key key) { return *reinterpret_cast<Value *>(Ins(Util::from(key))); }
    void clear() { FreeArray(); }
    Size size() const { return Count(); }
    Size count(Key first) const { return Count(Util::from(first)); }
    Size count(Key first, Key last) const { return Count(Util::from(first), Util::from(last)); }
    tl::optional<Entry> first() const { return Util::to<Entry>(First()); }
    tl::optional<Entry> first(Key key) const { return Util::to<Entry>(First(Util::from(key))); }
    tl::optional<Entry> next(Key key) const { return Util::to<Entry>(Next(Util::from(key))); }
    tl::optional<Entry> last() const { return Util::to<Entry>(Last()); }
    tl::optional<Entry> last(Key key) const { return Util::to<Entry>(Last(Util::from(key))); }
    tl::optional<Entry> prev(Key key) const { return Util::to<Entry>(Prev(Util::from(key))); }
    tl::optional<Key> firstEmpty() const { return Util::to<Key>(FirstEmpty()); }
    tl::optional<Key> firstEmpty(Key key) const { return Util::to<Key>(FirstEmpty(Util::from(key))); }
    tl::optional<Key> nextEmpty(Key key) const { return Util::to<Key>(NextEmpty(Util::from(key))); }
    tl::optional<Key> lastEmpty() const { return Util::to<Key>(LastEmpty()); }
    tl::optional<Key> lastEmpty(Key key) const { return Util::to<Key>(LastEmpty(Util::from(key))); }
    tl::optional<Key> prevEmpty(Key key) const { return Util::to<Key>(PrevEmpty(Util::from(key))); }
};

template<typename Key>
class Judy<Key, bool> : Judy1 {
    static_assert(sizeof(Key) <= sizeof(Word), "Key type must fit in pointer-width storage");
    static_assert(std::is_trivial<Key>::value, "Key must be trivial");
public:
    using key_type = Key;
    using value_type = bool;
    bool operator[](Key key) const { return Test(Util::from(key)); }
    Util::Ref1 operator[](Key key) { return Util::Ref1(*this, Util::from(key)); }
    void clear() { FreeArray(); }
    Size size() const { return Count(); }
    Size count(Key first) const { return Count(Util::from(first)); }
    Size count(Key first, Key last) const { return Count(Util::from(first), Util::from(last)); }
    tl::optional<Key> first() const { return Util::to<Key>(First()); }
    tl::optional<Key> first(Key key) const { return Util::to<Key>(First(Util::from(key))); }
    tl::optional<Key> next(Key key) const { return Util::to<Key>(Next(Util::from(key))); }
    tl::optional<Key> last() const { return Util::to<Key>(Last()); }
    tl::optional<Key> last(Key key) const { return Util::to<Key>(Last(Util::from(key))); }
    tl::optional<Key> prev(Key key) const { return Util::to<Key>(Prev(Util::from(key))); }
    tl::optional<Key> firstEmpty() const { return Util::to<Key>(FirstEmpty()); }
    tl::optional<Key> firstEmpty(Key key) const { return Util::to<Key>(FirstEmpty(Util::from(key))); }
    tl::optional<Key> nextEmpty(Key key) const { return Util::to<Key>(NextEmpty(Util::from(key))); }
    tl::optional<Key> lastEmpty() const { return Util::to<Key>(LastEmpty()); }
    tl::optional<Key> lastEmpty(Key key) const { return Util::to<Key>(LastEmpty(Util::from(key))); }
    tl::optional<Key> prevEmpty(Key key) const { return Util::to<Key>(PrevEmpty(Util::from(key))); }
};

} // end namespace Judy

#endif
