#include "Judy++.h"
#include <Judy.h>

#include <cstdint>

namespace Judy {

namespace {
tl::optional<Word> searchIndex(int result, Word &index) {
    if (result)
        return index;
    return tl::nullopt;
}
tl::optional<JudyL::Entry> searchEntry(PPvoid_t result, Word &index) {
    if (result)
        return std::make_pair(index, reinterpret_cast<Word *>(result));
    return tl::nullopt;
}
}

bool Judy1::Test(Word index) const { return Judy1Test(array, index, PJE0); }
bool Judy1::Set(Word index) { return Judy1Set(&array, index, PJE0); }
void Judy1::SetArray(const Word *indices, Size count) { Judy1SetArray(&array, count, indices, PJE0); }
bool Judy1::Unset(Word index) { return Judy1Unset(&array, index, PJE0); }
auto Judy1::Count(Word first, Word last) const -> Size { return Judy1Count(array, first, last, PJE0); }
auto Judy1::ByCount(Word count) const -> tl::optional<Word> { return searchIndex(Judy1ByCount(array, count, &count, PJE0), count); }
auto Judy1::FreeArray() -> Size { return Judy1FreeArray(&array, PJE0); }
auto Judy1::MemUsed() const -> Size { return Judy1MemUsed(array); }
auto Judy1::MemActive() const -> Size { return Judy1MemActive(array); }
auto Judy1::First(Word index) const -> tl::optional<Word> { return searchIndex(Judy1First(array, &index, PJE0), index); }
auto Judy1::Next(Word index) const -> tl::optional<Word> { return searchIndex(Judy1Next(array, &index, PJE0), index); }
auto Judy1::Last(Word index) const -> tl::optional<Word> { return searchIndex(Judy1Last(array, &index, PJE0), index); }
auto Judy1::Prev(Word index) const -> tl::optional<Word> { return searchIndex(Judy1Prev(array, &index, PJE0), index); }
auto Judy1::FirstEmpty(Word index) const -> tl::optional<Word> { return searchIndex(Judy1FirstEmpty(array, &index, PJE0), index); }
auto Judy1::NextEmpty(Word index) const -> tl::optional<Word> { return searchIndex(Judy1NextEmpty(array, &index, PJE0), index); }
auto Judy1::LastEmpty(Word index) const -> tl::optional<Word> { return searchIndex(Judy1LastEmpty(array, &index, PJE0), index); }
auto Judy1::PrevEmpty(Word index) const -> tl::optional<Word> { return searchIndex(Judy1PrevEmpty(array, &index, PJE0), index); }

Word *JudyL::Get(Word index) { return reinterpret_cast<Word *>(JudyLGet(array, index, PJE0)); }
const Word *JudyL::Get(Word index) const { return reinterpret_cast<Word *>(JudyLGet(array, index, PJE0)); }
Word *JudyL::Ins(Word index) { return reinterpret_cast<Word *>(JudyLIns(&array, index, PJE0)); }
void JudyL::InsArray(const Word *indices, const Word *values, Size count) { JudyLInsArray(&array, count, indices, values, PJE0); }
bool JudyL::Del(Word index) { return JudyLDel(&array, index, PJE0); }
Size JudyL::Count(Word first, Word last) const { return JudyLCount(array, first, last, PJE0); }
auto JudyL::ByCount(Word index) const -> tl::optional<Entry> { return searchEntry(JudyLByCount(array, index, &index, PJE0), index); }
Size JudyL::FreeArray() { return JudyLFreeArray(&array, PJE0); }
Size JudyL::MemUsed() const { return JudyLMemUsed(array); }
Size JudyL::MemActive() const { return JudyLMemActive(array); }
auto JudyL::First(Word index) const -> tl::optional<Entry> { return searchEntry(JudyLFirst(array, &index, PJE0), index); }
auto JudyL::Next(Word index) const -> tl::optional<Entry> { return searchEntry(JudyLNext(array, &index, PJE0), index); }
auto JudyL::Last(Word index) const -> tl::optional<Entry> { return searchEntry(JudyLLast(array, &index, PJE0), index); }
auto JudyL::Prev(Word index) const -> tl::optional<Entry> { return searchEntry(JudyLPrev(array, &index, PJE0), index); }
auto JudyL::FirstEmpty(Word index) const -> tl::optional<Word> { return searchIndex(JudyLFirstEmpty(array, &index, PJE0), index); }
auto JudyL::NextEmpty(Word index) const -> tl::optional<Word> { return searchIndex(JudyLNextEmpty(array, &index, PJE0), index); }
auto JudyL::LastEmpty(Word index) const -> tl::optional<Word> { return searchIndex(JudyLLastEmpty(array, &index, PJE0), index); }
auto JudyL::PrevEmpty(Word index) const -> tl::optional<Word> { return searchIndex(JudyLPrevEmpty(array, &index, PJE0), index); }

bool JudyL::invalid() const noexcept { return std::uintptr_t(array) & JLAP_INVALID; }
Pvoid_t JudyL::user() const noexcept { return reinterpret_cast<Pvoid_t>(std::uintptr_t(array) & ~JLAP_INVALID); }

}
