#include "debuginfo.h"

#define MY_DEBUG

#ifdef MY_DEBUG
#include <QtCore/QDebug>
#include <QtCore/QString>
#endif

namespace debuginfo {
namespace {
template<typename Type> constexpr Type consume(Data &data) noexcept {
    auto res = data.complete<Type>();
    data = data.subview(sizeof(Type));
    return res;
}
_Str consume_strz(Data &data) noexcept {
    _Str str = data.bitcast<_Str::element_type>();
    auto end = std::find(str.begin(), str.end(), '\0');
    if (end == str.end()) {
        return "";
    }
    return data.take_first(end - str.begin() + 1).bitcast<_Str::element_type>();
}
} // end anonymous namespace

namespace elf {
enum {
    ELFCLASS32          = 1,
    ELFDATA2LSB         = 1,
    ELFOSABI_STANDALONE = 255,
    ET_EXEC             = 2,
    EM_Z80              = 220,
    EF_Z80_EZ80         = 1,
    EV_CURRENT          = 1,
    SHT_NULL            = 0,
    SHT_PROGBITS        = 1,
    SHT_SYMTAB          = 2,
    SHT_STRTAB          = 3,
    SHT_NOBITS          = 8,
    SHF_WRITE           = 1 << 0,
    SHF_ALLOC           = 1 << 1,
    SHF_EXECINSTR       = 1 << 2,
    SHF_MERGE           = 1 << 4,
    SHF_STRINGS         = 1 << 5,
};

const File::Header File::Header::ez80 {
    .ei_mag = { '\x7F', 'E','L','F' },
    .ei_class = ELFCLASS32,
    .ei_data = ELFDATA2LSB,
    .ei_version = EV_CURRENT,
    .ei_osabi = ELFOSABI_STANDALONE,
    .e_type = ET_EXEC,
    .e_machine = EM_Z80,
    .e_version = EV_CURRENT,
    .e_flags = EF_Z80_EZ80,
};


Data ProgramHeader::data(const File &file) const noexcept {
    return file._M_data.subview(p_offset, p_filesz);
}

_Str SectionHeader::name(const File &file) const noexcept {
    auto name = file.section_headers()[file.header().e_shstrndx].data(file).subview(sh_name);
    return consume_strz(name);
}
Data SectionHeader::data(const File &file) const noexcept {
    if (sh_type == SHT_NOBITS)
        return {};
    return file._M_data.subview(sh_offset, sh_size);
}
} // end namespace elf

namespace dwarf {
void File::validate(const Header &expected) {
    if (!elf::File::validate(expected))
        return error("Invalid ELF header");
    for (const auto &sh : section_headers()) {
        auto name = sh.name(*this);
        if (name == ".debug_abbrev")
            _M_abbrev.data(sh.data(*this));
        else if (name == ".debug_addr")
            _M_addr.data(sh.data(*this));
        else if (name == ".debug_frame")
            _M_frame.data(sh.data(*this));
        else if (name == ".debug_info")
            _M_info.data(sh.data(*this));
        else if (name == ".debug_line")
            _M_line.data(sh.data(*this));
        else if (name == ".debug_line_str")
            _M_line_str.data(sh.data(*this));
        else if (name == ".debug_line_str_offsets")
            _M_line_str_offsets.data(sh.data(*this));
        else if (name == ".debug_loclists")
            _M_loclists.data(sh.data(*this));
        else if (name == ".debug_rnglists")
            _M_rnglists.data(sh.data(*this));
        else if (name == ".debug_str")
            _M_str.data(sh.data(*this));
        else if (name == ".debug_str_offsets")
            _M_str_offsets.data(sh.data(*this));
    }
    _M_frame.__parse();
    _M_info.__parse(_M_line, _M_loclists, _M_rnglists);
}

namespace section {
namespace {
template<typename Enum> constexpr
std::enable_if_t<std::is_enum<Enum>::value, std::underlying_type_t<Enum>> underlying(Enum e) noexcept {
    return std::underlying_type_t<Enum>(e);
}
constexpr _SizedIntegral<std::uintmax_t> consume_uleb128(Data &data) noexcept {
    _Sbyte cur{};
    std::uintmax_t res{};
    std::size_t shift{};
    do {
        cur = consume<_Sbyte>(data);
        res |= (cur & 0x7F) << shift;
        shift += std::numeric_limits<_Sbyte>::digits;
    } while (cur < 0);
    return { res, shift };
}
constexpr _SizedIntegral<std::intmax_t> consume_sleb128(Data &data) noexcept {
    _Sbyte cur{};
    std::intmax_t res{};
    std::size_t shift{};
    do {
        cur = consume<_Sbyte>(data);
        res |= (cur & 0x7F) << shift;
        shift += std::numeric_limits<_Sbyte>::digits;
    } while (cur < 0);
    if (shift <= std::numeric_limits<std::intmax_t>::digits) {
        res = res << (std::numeric_limits<std::intmax_t>::digits - shift + 1)
                  >> (std::numeric_limits<std::intmax_t>::digits - shift + 1);
    }
    return { res, shift };
}
_Val consume_form(const Abbrev::_Attr &attr, Data &data, _Form &fixup) noexcept {
    switch (attr._M_at) {
        default:
            break;
        case _At::DW_AT_high_pc:
            fixup = _Form::_HighPC;
            break;
        case _At::DW_AT_decl_file:
            fixup = _Form::_DeclFile;
            break;
    }

    switch (auto cur = attr._M_form) {
        case _Form::DW_FORM_strx3:
        case _Form::DW_FORM_addrx3:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_addr:
            return consume<_Addr>(data);
        case _Form::DW_FORM_block2:
            return data.take_first(consume<_Half>(data));
        case _Form::DW_FORM_block4:
            return data.take_first(consume<_Word>(data));
        case _Form::DW_FORM_ref2:
        case _Form::DW_FORM_strx2:
        case _Form::DW_FORM_addrx2:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_data2:
            return consume<_Half>(data);
        case _Form::DW_FORM_ref4:
        case _Form::DW_FORM_strx4:
        case _Form::DW_FORM_addrx4:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_data4:
            return consume<_Word>(data);
        case _Form::DW_FORM_ref8:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_data8:
            return consume<_Long>(data);
        case _Form::DW_FORM_string:
            return consume_strz(data);
        case _Form::DW_FORM_block:
        case _Form::DW_FORM_exprloc:
            return data.take_first(consume_uleb128(data));
        case _Form::DW_FORM_block1:
            return data.take_first(consume<_Byte>(data));
        case _Form::DW_FORM_ref1:
        case _Form::DW_FORM_strx1:
        case _Form::DW_FORM_addrx1:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_data1:
            return consume<_Byte>(data);
        case _Form::DW_FORM_flag:
            return bool(consume<_Byte>(data));
        case _Form::DW_FORM_sdata:
            return consume_sleb128(data);
        case _Form::DW_FORM_strp:
        case _Form::DW_FORM_ref_addr:
        case _Form::DW_FORM_line_strp:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_sec_offset:
            return consume<_Off>(data);
        case _Form::DW_FORM_ref_udata:
        case _Form::DW_FORM_strx:
        case _Form::DW_FORM_addrx:
        case _Form::DW_FORM_loclistx:
        case _Form::DW_FORM_rnglistx:
            fixup = cur;
            [[gnu::fallthrough]];
        case _Form::DW_FORM_udata:
            return consume_uleb128(data);
        case _Form::DW_FORM_indirect:
            if (fixup == _Form::_None) {
                auto form = consume_uleb128(data);
                if (form <= underlying(_Form::_None) || form >= underlying(_Form::_HighPC))
                    return {};
                auto val = consume_form({ _At::_None, _Form(form.get()), {} }, data, cur);
                if (cur != _Form::DW_FORM_indirect)
                    fixup = cur;
                return val;
            }
            [[gnu::fallthrough]];
        default:
            return {};
        case _Form::DW_FORM_flag_present:
            return true;
        case _Form::DW_FORM_data16:
            return data.take_first(16);
        case _Form::DW_FORM_implicit_const:
            if (attr._M_at == _At::_None)
                return {};
            return attr._M_implicit_const;
    }
}
_Str strx(_Word index, const _Die &unit_entry, const File &file) noexcept {
    auto str_offsets = file.str_offsets().get(unit_entry);
    if (index >= str_offsets.size()) {
        file.error("Out of range index into str offsets");
        return {};
    }
    return file.str().get(str_offsets[index]);
}
_Addr addrx(_Word index, const _Die &unit_entry, const File &file) noexcept {
    auto addr = file.addr().get(unit_entry);
    if (index >= addr.size()) {
        file.error("Out of range index into addr");
        return {};
    }
    return addr[index];
}

#ifdef MY_DEBUG
template<typename Type, std::size_t Extent> constexpr std::size_t size(const Type (&)[Extent]) noexcept {
    return Extent;
}

QDebug operator<<[[gnu::unused]](QDebug debug, _Tag tag) {
    static const char *names[] = {
        "_None",
        "DW_TAG_array_type",
        "DW_TAG_class_type",
        "DW_TAG_entry_point",
        "DW_TAG_enumeration_type",
        "DW_TAG_formal_parameter",
        "_Reserved",
        "_Reserved",
        "DW_TAG_imported_declaration",
        "_Reserved",
        "DW_TAG_label",
        "DW_TAG_lexical_block",
        "_Reserved",
        "DW_TAG_member",
        "_Reserved",
        "DW_TAG_pointer_type",
        "DW_TAG_reference_type",
        "DW_TAG_compile_unit",
        "DW_TAG_string_type",
        "DW_TAG_structure_type",
        "_Reserved",
        "DW_TAG_subroutine_type",
        "DW_TAG_typedef",
        "DW_TAG_union_type",
        "DW_TAG_unspecified_parameters",
        "DW_TAG_variant",
        "DW_TAG_common_block",
        "DW_TAG_common_inclusion",
        "DW_TAG_inheritance",
        "DW_TAG_inlined_subroutine",
        "DW_TAG_module",
        "DW_TAG_ptr_to_member_type",
        "DW_TAG_set_type",
        "DW_TAG_subrange_type",
        "DW_TAG_with_stmt",
        "DW_TAG_access_declaration",
        "DW_TAG_base_type",
        "DW_TAG_catch_block",
        "DW_TAG_const_type",
        "DW_TAG_constant",
        "DW_TAG_enumerator",
        "DW_TAG_file_type",
        "DW_TAG_friend",
        "DW_TAG_namelist",
        "DW_TAG_namelist_item",
        "DW_TAG_packed_type",
        "DW_TAG_subprogram",
        "DW_TAG_template_type_parameter",
        "DW_TAG_template_value_parameter",
        "DW_TAG_thrown_type",
        "DW_TAG_try_block",
        "DW_TAG_variant_part",
        "DW_TAG_variable",
        "DW_TAG_volatile_type",
        "DW_TAG_dwarf_procedure",
        "DW_TAG_restrict_type",
        "DW_TAG_interface_type",
        "DW_TAG_namespace",
        "DW_TAG_imported_module",
        "DW_TAG_unspecified_type",
        "DW_TAG_partial_unit",
        "DW_TAG_imported_unit",
        "_Reserved",
        "DW_TAG_condition",
        "DW_TAG_shared_type",
        "DW_TAG_type_unit",
        "DW_TAG_rvalue_reference_type",
        "DW_TAG_template_alias",
        "DW_TAG_coarray_type",
        "DW_TAG_generic_subrange",
        "DW_TAG_dynamic_type",
        "DW_TAG_atomic_type",
        "DW_TAG_call_site",
        "DW_TAG_call_site_parameter",
        "DW_TAG_skeleton_unit",
        "DW_TAG_immutable_type",
        "_Unknown",
    };
    static_assert(size(names) == std::size_t(_Tag::_Unused), "Missing names");
    QDebugStateSaver saver(debug);
    return debug.nospace().noquote() << Qt::left << qSetFieldWidth(31)
                                     << (_Word(tag) < size(names) ? names[_Word(tag)] : "_Reserved");
}
QDebug operator<<[[gnu::unused]](QDebug debug, _At at) {
    const char *names[] = {
        "_None",
        "DW_AT_sibling",
        "DW_AT_location",
        "DW_AT_name",
        "_Reserved",
        "_Reserved",
        "_Reserved",
        "_Reserved",
        "_Reserved",
        "DW_AT_ordering",
        "_Reserved",
        "DW_AT_byte_size",
        "_Reserved",
        "DW_AT_bit_size",
        "_Reserved",
        "_Reserved",
        "DW_AT_stmt_list",
        "DW_AT_low_pc",
        "DW_AT_high_pc",
        "DW_AT_language",
        "_Reserved",
        "DW_AT_discr",
        "DW_AT_discr_value",
        "DW_AT_visibility",
        "DW_AT_import",
        "DW_AT_string_length",
        "DW_AT_common_reference",
        "DW_AT_comp_dir",
        "DW_AT_const_value",
        "DW_AT_containing_type",
        "DW_AT_default_value",
        "_Reserved",
        "DW_AT_inline",
        "DW_AT_is_optional",
        "DW_AT_lower_bound",
        "_Reserved",
        "_Reserved",
        "DW_AT_producer",
        "_Reserved",
        "DW_AT_prototyped",
        "_Reserved",
        "_Reserved",
        "DW_AT_return_addr",
        "_Reserved",
        "DW_AT_start_scope",
        "_Reserved",
        "DW_AT_bit_stride",
        "DW_AT_upper_bound",
        "_Reserved",
        "DW_AT_abstract_origin",
        "DW_AT_accessibility",
        "DW_AT_address_class",
        "DW_AT_artificial",
        "DW_AT_base_types",
        "DW_AT_calling_convention",
        "DW_AT_count",
        "DW_AT_data_member_location",
        "DW_AT_decl_column",
        "DW_AT_decl_file",
        "DW_AT_decl_line",
        "DW_AT_declaration",
        "DW_AT_discr_list",
        "DW_AT_encoding",
        "DW_AT_external",
        "DW_AT_frame_base",
        "DW_AT_friend",
        "DW_AT_identifier_case",
        "_Reserved",
        "DW_AT_namelist_item",
        "DW_AT_priority",
        "DW_AT_segment",
        "DW_AT_specification",
        "DW_AT_static_link",
        "DW_AT_type",
        "DW_AT_use_location",
        "DW_AT_variable_parameter",
        "DW_AT_virtuality",
        "DW_AT_vtable_elem_location",
        "DW_AT_allocated",
        "DW_AT_associated",
        "DW_AT_data_location",
        "DW_AT_byte_stride",
        "DW_AT_entry_pc",
        "DW_AT_use_UTF8",
        "DW_AT_extension",
        "DW_AT_ranges",
        "DW_AT_trampoline",
        "DW_AT_call_column",
        "DW_AT_call_file",
        "DW_AT_call_line",
        "DW_AT_description",
        "DW_AT_binary_scale",
        "DW_AT_decimal_scale",
        "DW_AT_small",
        "DW_AT_decimal_sign",
        "DW_AT_digit_count",
        "DW_AT_picture_string",
        "DW_AT_mutable",
        "DW_AT_threads_scaled",
        "DW_AT_explicit",
        "DW_AT_object_pointer",
        "DW_AT_endianity",
        "DW_AT_elemental",
        "DW_AT_pure",
        "DW_AT_recursive",
        "DW_AT_signature",
        "DW_AT_main_subprogram",
        "DW_AT_data_bit_offset",
        "DW_AT_const_expr",
        "DW_AT_enum_class",
        "DW_AT_linkage_name",
        "DW_AT_string_length_bit_size",
        "DW_AT_string_length_byte_size",
        "DW_AT_rank",
        "DW_AT_str_offsets_base",
        "DW_AT_addr_base",
        "DW_AT_rnglists_base",
        "_Reserved",
        "DW_AT_dwo_name",
        "DW_AT_reference",
        "DW_AT_rvalue_reference",
        "DW_AT_macros",
        "DW_AT_call_all_calls",
        "DW_AT_call_all_source_calls",
        "DW_AT_call_all_tail_calls",
        "DW_AT_call_return_pc",
        "DW_AT_call_value",
        "DW_AT_call_origin",
        "DW_AT_call_parameter",
        "DW_AT_call_pc",
        "DW_AT_call_tail_call",
        "DW_AT_call_target",
        "DW_AT_call_target_clobbered",
        "DW_AT_call_data_location",
        "DW_AT_call_data_value",
        "DW_AT_noreturn",
        "DW_AT_alignment",
        "DW_AT_export_symbols",
        "DW_AT_deleted",
        "DW_AT_defaulted",
        "DW_AT_loclists_base",
        "_Unknown",
    };
    static_assert(size(names) == std::size_t(_At::_Unused), "Missing names");
    QDebugStateSaver saver(debug);
    return debug.nospace().noquote() << Qt::left << qSetFieldWidth(30)
                                     << (_Word(at) < size(names) ? names[_Word(at)] : "_Reserved");
}
QDebug operator<<[[gnu::used]](QDebug debug, _Form form) {
    const char *names[] = {
        "_None",
        "DW_FORM_addr",
        "_Reserved",
        "DW_FORM_block2",
        "DW_FORM_block4",
        "DW_FORM_data2",
        "DW_FORM_data4",
        "DW_FORM_data8",
        "DW_FORM_string",
        "DW_FORM_block",
        "DW_FORM_block1",
        "DW_FORM_data1",
        "DW_FORM_flag",
        "DW_FORM_sdata",
        "DW_FORM_strp",
        "DW_FORM_udata",
        "DW_FORM_ref_addr",
        "DW_FORM_ref1",
        "DW_FORM_ref2",
        "DW_FORM_ref4",
        "DW_FORM_ref8",
        "DW_FORM_ref_udata",
        "DW_FORM_indirect",
        "DW_FORM_sec_offset",
        "DW_FORM_exprloc",
        "DW_FORM_flag_present",
        "DW_FORM_strx",
        "DW_FORM_addrx",
        "DW_FORM_ref_sup4",
        "DW_FORM_strp_sup",
        "DW_FORM_data16",
        "DW_FORM_line_strp",
        "DW_FORM_ref_sig8",
        "DW_FORM_implicit_const",
        "DW_FORM_loclistx",
        "DW_FORM_rnglistx",
        "DW_FORM_ref_sup8",
        "DW_FORM_strx1",
        "DW_FORM_strx2",
        "DW_FORM_strx3",
        "DW_FORM_strx4",
        "DW_FORM_addrx1",
        "DW_FORM_addrx2",
        "DW_FORM_addrx3",
        "DW_FORM_addrx4",
        "_HighPC",
        "_File",
    };
    static_assert(size(names) == std::size_t(_Form::_Unused), "Missing names");
    QDebugStateSaver saver(debug);
    return debug.nospace().noquote() << Qt::left << qSetFieldWidth(29)
                                     << (_Word(form) < size(names) ? names[_Word(form)] : "_Reserved");
}
QDebug operator<<[[gnu::unused]](QDebug debug, _Str str) {
    return debug << QString::fromUtf8(str.data(), str.rtrim('\0').size());
}
template<std::size_t MaxParts> QDebug operator<<[[gnu::unused]](QDebug debug,
                                                                const _Path<MaxParts> &path) {
    QString str;
    for (auto c : path.canon())
        str += c;
    return debug << str;
}
QDebug operator<<[[gnu::unused]](QDebug debug, _Addr addr) {
    QDebugStateSaver saver(debug);
    return debug << Qt::hex << Qt::showbase << _Word(addr);
}
QDebug operator<<[[gnu::unused]](QDebug debug, const _Die &entry);
QDebug operator<<[[gnu::unused]](QDebug debug, const _Val &val) {
    QDebugStateSaver saver(debug);
    if (val.is_none())
        return debug.noquote() << "<none>";
    if (val.is_addr())
        return debug << val.addr();
    if (val.is_cst())
        return debug << Qt::hex << Qt::showbase << val.cst();
    if (val.is_data())
        return debug.noquote() << QByteArray::fromRawData(
                val.data().bitcast<_Str::element_type>().data(), val.data().size()).toHex(' ');
    if (val.is_file())
        return debug.quote() << val.file().path();
    if (val.is_flag())
        return debug << val.flag();
    if (val.is_ref())
        return debug.quote() << val.ref()._M_tag << val.ref();
    if (val.is_str())
        return debug.quote() << val.str();
    debug.nospace();
    if (val.is_loclist()) {
        bool first = true;
        for (auto loc : val.loclist()) {
            if (first)
                first = false;
            else
                debug << ", ";
            debug << '[' << loc._M_range.first << ',' << loc._M_range.second << ") " << loc._M_expr;
        }
        return debug;
    }
    if (val.is_rnglist()) {
        bool first = true;
        for (auto rng : val.rnglist()) {
            if (first)
                first = false;
            else
                debug << ", ";
            debug << '[' << rng.first << ',' << rng.second << ')';
        }
        return debug;
    }
    return debug << "<unknown>";
}
QDebug operator<<[[gnu::unused]](QDebug debug, const _Die &entry) {
    auto name = std::cref(entry.attr(_At::DW_AT_name));
    if (name.get().is_none())
        name = entry.attr(_At::DW_AT_abstract_origin).ref().attr(_At::DW_AT_name);
    return debug << name;
}
#endif
} // end anonymous namespace

const SrcFile SrcFile::_S_null;

const _Val _Val::_S_none;

const _Val &_Die::attr(_At at) const noexcept {
    auto attr = _M_attrs.find(at);
    if (attr != _M_attrs.end())
        return attr->second;
    attr = _M_attrs.find(_At::DW_AT_abstract_origin);
    if (attr != _M_attrs.end())
        return attr->second.ref().attr(at);
    return _Val::_S_none;
}

const _Die _Die::_S_null{};

auto Abbrev::get(_Off base) const noexcept -> std::unordered_map<_Word, _Entry> {
    std::unordered_map<_Word, _Entry> abbrevs;
    auto unit = _M_data.subview(base);
    while (!error()) {
        auto code = consume_uleb128(unit);
        if (!code) {
            return abbrevs;
        }
        if (_Slong(code) != _Sword(code)) {
            error("Unsupported code value size");
            return {};
        }
        _Entry entry;
        auto tag = consume_uleb128(unit);
        switch (tag) {
            case underlying(_Tag::_None):
                error("Invalid tag");
                return {};
            default:
#ifdef MY_DEBUG
                if (tag >= underlying(_Tag::_Unknown)) {
                    qWarning() << "Unknown TAG" << Qt::hex << Qt::showbase << tag.get();
                }
#endif
                entry._M_tag = _Tag(std::min(tag, decltype(tag)(_Tag::_Unknown)).get());
                break;
            // Vendor Extensions
            case DW_TAG_GNU_call_site:
                entry._M_tag = _Tag::DW_TAG_call_site;
                break;
            case DW_TAG_GNU_call_site_parameter:
                entry._M_tag = _Tag::DW_TAG_call_site_parameter;
                break;
        }
        entry._M_children = consume<_Byte>(unit);
#ifdef MY_DEBUG_DISABLED
        qDebug().nospace() << '[' << code.get() << "] " << entry._M_tag
                           << (entry._M_children ? " DW_CHILDREN_yes" : " DW_CHILDREN_no");
#endif
        while (!error()) {
            auto type = consume_uleb128(unit), form = consume_uleb128(unit);
            if (!type && !form) {
                break;
            }
            auto at = _At(type.get());
            switch (type) {
                case underlying(_At::_None):
                    error("Invalid at");
                    return {};
                case underlying(_At::DW_AT_low_pc):
                    switch (tag) {
                        default:
                            at = _At::DW_AT_low_pc;
                            break;
                        case DW_TAG_GNU_call_site:
                            at = _At::DW_AT_call_return_pc;
                            break;
                    }
                    break;
                case underlying(_At::DW_AT_abstract_origin):
                    switch (tag) {
                        default:
                            at = _At::DW_AT_abstract_origin;
                            break;
                        case DW_TAG_GNU_call_site:
                            at = _At::DW_AT_call_origin;
                            break;
                    }
                    break;
                default:
#ifdef MY_DEBUG
                    if (type >= underlying(_At::_Unknown)) {
                        qWarning() << "Unknown AT" << Qt::hex << Qt::showbase << at;
                    }
#endif
                    at = _At(std::min(type, decltype(type)(_At::_Unknown)).get());
                    break;
                // Vendor Extensions
                case DW_AT_GNU_call_site_value:
                    at = _At::DW_AT_call_value;
                    break;
                case DW_AT_GNU_call_site_target:
                    at = _At::DW_AT_call_target;
                    break;
                case DW_AT_GNU_tail_call:
                    at = _At::DW_AT_call_tail_call;
                    break;
                case DW_AT_GNU_all_call_sites:
                    at = _At::DW_AT_call_all_calls;
                    break;
            }
            if (form <= underlying(_Form::_None) || form >= underlying(_Form::_HighPC)) {
                error("Out of range abbrev attr form");
                return {};
            }
            _Slong implicit_const{};
            if (_Form(form.get()) == _Form::DW_FORM_implicit_const) {
                implicit_const = consume_sleb128(unit);
            }
            entry._M_attrs.push_back({ std::min(_At(at), _At::_Unknown),
                                       _Form(form.get()), implicit_const });
#ifdef MY_DEBUG_DISABLED
            qDebug() << '\t' << entry._M_attrs.back()._M_at << entry._M_attrs.back()._M_form;
#endif
        }
#ifdef MY_DEBUG_DISABLED
        qDebug() << "";
#endif
        entry._M_attrs.shrink_to_fit();
        abbrevs.emplace(code, std::move(entry));
    }
    return {};
}

_AddrView Addr::get(const _Die &unit_entry) const noexcept {
    auto base = unit_entry.attr(_At::DW_AT_addr_base);
    if (base.is_none()) {
        return {};
    }
    auto unit = _M_data.subview(base.cst() - sizeof(_Word)
                                - sizeof(_Half) - sizeof(_Byte) - sizeof(_Byte));
    auto unit_length = consume<_Word>(unit);
    unit = unit.first(unit_length);
    auto version = consume<_Half>(unit);
    if (version < 5 || version > 5) {
        error("Unsupported addr version");
        return {};
    }
    auto address_size = consume<_Byte>(unit);
    if (address_size != sizeof(_Addr)) {
        error("Unsupported address size");
        return {};
    }
    auto segment_selector_size = consume<_Byte>(unit);
    if (segment_selector_size) {
        error("Unsupported segment selector size");
        return {};
    }
    return unit.bitcast<const _Addr>();
}

const Frame::RuleSet Frame::RuleSet::_S_undef;

void Frame::__parse() {
    // Table 7.29: Call frame instruction encodings
    enum {
        DW_CFA_advance_loc        = 0x1,
        DW_CFA_offset             = 0x2,
        DW_CFA_restore            = 0x3,
        DW_CFA_nop                = 0,
        DW_CFA_set_loc            = 0x01,
        DW_CFA_advance_loc1       = 0x02,
        DW_CFA_advance_loc2       = 0x03,
        DW_CFA_advance_loc4       = 0x04,
        DW_CFA_offset_extended    = 0x05,
        DW_CFA_restore_extended   = 0x06,
        DW_CFA_undefined          = 0x07,
        DW_CFA_same_value         = 0x08,
        DW_CFA_register           = 0x09,
        DW_CFA_remember_state     = 0x0a,
        DW_CFA_restore_state      = 0x0b,
        DW_CFA_def_cfa            = 0x0c,
        DW_CFA_def_cfa_register   = 0x0d,
        DW_CFA_def_cfa_offset     = 0x0e,
        DW_CFA_def_cfa_expression = 0x0f,
        DW_CFA_expression         = 0x10,
        DW_CFA_offset_extended_sf = 0x11,
        DW_CFA_def_cfa_sf         = 0x12,
        DW_CFA_def_cfa_offset_sf  = 0x13,
        DW_CFA_val_offset         = 0x14,
        DW_CFA_val_offset_sf      = 0x15,
        DW_CFA_val_expression     = 0x16,
        // Vendor Extensions
        DW_CFA_lo_user            = 0x1c,
        DW_CFA_hi_user            = 0x3f,
    };
    struct CIE {
        _Word code_alignment_factor;
        _Sword data_alignment_factor;
        RuleSet initial_rules;
    };
    std::unordered_map<_Off, CIE> cies;

    const auto evaluate_instructions = [&](const CIE &cie, RuleSet &rules, Data instructions,
                                           _Word location = _InvalidWord) {
        const auto set_loc = [&](_Addr new_location) {
            if (location == _InvalidWord)
                return error("Invalid location in initial instructions");
            _M_rules[std::exchange(location, new_location)] = rules;
        };
        const auto advance_loc = [&](_Word advance) {
            set_loc(location + advance * cie.code_alignment_factor);
        };
        const auto parse_reg = [&](_Long reg) -> Reg {
            if (reg >= decltype(reg)(Reg::None)) {
                error("Unknown register");
                return Reg::None;
            }
            return Reg(reg);
        };
        std::vector<RuleSet> stack;
        while (!instructions.empty() && !error()) {
            Reg reg;
            switch (auto opcode = consume<_Byte>(instructions)) {
                case DW_CFA_nop:
                    break;
                case DW_CFA_set_loc:
                    set_loc(consume<_Addr>(instructions));
                    break;
                case DW_CFA_advance_loc1:
                    advance_loc(consume<_Byte>(instructions));
                    break;
                case DW_CFA_advance_loc2:
                    advance_loc(consume<_Half>(instructions));
                    break;
                case DW_CFA_advance_loc4:
                    advance_loc(consume<_Word>(instructions));
                    break;
                case DW_CFA_offset_extended:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::off(consume_uleb128(instructions)
                                           * cie.data_alignment_factor);
                    break;
                case DW_CFA_restore_extended:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = cie.initial_rules[reg];
                    break;
                case DW_CFA_undefined:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::undef();
                    break;
                case DW_CFA_same_value:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::same_val();
                    break;
                case DW_CFA_register:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::reg(parse_reg(consume_uleb128(instructions)));
                    break;
                case DW_CFA_remember_state:
                    stack.push_back(rules);
                    break;
                case DW_CFA_restore_state:
                    if (stack.empty())
                        return error("Underflow state stack");
                    rules = stack.back();
                    stack.pop_back();
                    break;
                case DW_CFA_def_cfa:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules.cfa() = Rule::reg_off(reg, consume_uleb128(instructions));
                    break;
                case DW_CFA_def_cfa_register:
                    rules.cfa() = Rule::reg_off(parse_reg(consume_uleb128(instructions)),
                                                rules.cfa().off());
                    break;
                case DW_CFA_def_cfa_offset:
                    rules.cfa() = Rule::reg_off(rules.cfa().reg(), consume_uleb128(instructions));
                    break;
                case DW_CFA_def_cfa_expression:
                    rules.cfa() = Rule::val_expr(*this, instructions.take_first(
                                                         consume_uleb128(instructions)));
                    break;
                case DW_CFA_expression:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::expr(*this, instructions.take_first(
                                                    consume_uleb128(instructions)));
                    break;
                case DW_CFA_offset_extended_sf:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::off(consume_sleb128(instructions)
                                           * cie.data_alignment_factor);
                    break;
                case DW_CFA_def_cfa_sf:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules.cfa() = Rule::reg_off(reg, consume_sleb128(instructions)
                                                * cie.data_alignment_factor);
                    break;
                case DW_CFA_def_cfa_offset_sf:
                    rules.cfa() = Rule::reg_off(rules.cfa().reg(), consume_sleb128(instructions)
                                                * cie.data_alignment_factor);
                    break;
                case DW_CFA_val_offset:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::val_off(consume_uleb128(instructions)
                                               * cie.data_alignment_factor);
                    break;
                case DW_CFA_val_offset_sf:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::val_off(consume_sleb128(instructions)
                                               * cie.data_alignment_factor);
                    break;
                case DW_CFA_val_expression:
                    reg = parse_reg(consume_uleb128(instructions));
                    rules[reg] = Rule::val_expr(*this, instructions.take_first(
                                                        consume_uleb128(instructions)));
                    break;
                default: {
                    decltype(opcode) info = opcode & ((1 << 6) - 1);
                    switch (opcode >> 6) {
                        case DW_CFA_advance_loc:
                            advance_loc(info);
                            break;
                        case DW_CFA_offset:
                            reg = parse_reg(info);
                            rules[reg] = Rule::off(consume_uleb128(instructions)
                                                   * cie.data_alignment_factor);
                            break;
                        case DW_CFA_restore:
                            reg = parse_reg(info);
                            rules[reg] = cie.initial_rules[reg];
                            break;
                        default:
#ifdef MY_DEBUG
                            qWarning() << "Unknown opcode" << Qt::hex << Qt::showbase << opcode;
#endif
                            return error("Unknown opcode");
                    }
                    break;
                }
            }
        }
        if (location != _InvalidWord) {
            _M_rules[location] = rules;
        }
    };

    Data data = _M_data;
    while (!data.empty() && !error()) {
        _Off entry_offset = std::distance(_M_data.begin(), data.begin());
        auto length = consume<_Word>(data);
        auto entry = data.take_first(length);
        auto cie_offset = consume<_Off>(entry);
        if (cie_offset == _InvalidOff) {
            auto &cie = cies[entry_offset];
            auto version = consume<_Byte>(entry);
            if (version < 1 || version == 2 || version > 4)
                return error("Unsupported frame version");
            auto augmentation = consume_strz(entry);
            if (augmentation != "")
                return error("Unsupported augmentation");
            auto address_size = consume<_Byte>(entry);
            if (address_size != sizeof(_Addr))
                return error("Unsupported address size");
            auto segment_selector_size = consume<_Byte>(entry);
            if (segment_selector_size)
                return error("Unsupported segment selector size");
            auto code_alignment_factor = consume_uleb128(entry);
            cie.code_alignment_factor = code_alignment_factor;
            if (cie.code_alignment_factor != code_alignment_factor)
                return error("Unsupported code alignment factor");
            auto data_alignment_factor = consume_sleb128(entry);
            cie.data_alignment_factor = data_alignment_factor;
            if (cie.data_alignment_factor != data_alignment_factor)
                return error("Unsupported data alignment factor");
            auto return_address_register = consume_uleb128(entry);
            if (return_address_register >= decltype(return_address_register)(Reg::None))
                return error("Out of range register");
            auto initial_instructions = entry;
            evaluate_instructions(cie, cie.initial_rules, initial_instructions);
        } else {
            auto it = cies.find(cie_offset);
            if (it == cies.end())
                return error("Invalid CIE_pointer");
            const auto &cie = it->second;
            auto initial_location = consume<_Addr>(entry);
            auto address_range = consume<_Addr>(entry);
            auto instructions = entry;
            RuleSet rules = cie.initial_rules;
            evaluate_instructions(cie, rules, instructions, initial_location);
            _M_rules.emplace(initial_location + address_range, RuleSet::undef());
        }
    }
}

auto Frame::get(_Addr addr) const noexcept -> const RuleSet & {
    auto it = _M_rules.upper_bound(addr);
    if (it == _M_rules.begin())
        return RuleSet::undef();
    return (--it)->second;
}

auto Info::_Scope::get(_Addr addr) const noexcept -> const _Scope & {
    auto it = _M_children.upper_bound(addr);
    if (it == _M_children.begin() || addr >= (--it)->second.first)
        return _Scope::_S_null;
    return it->second.second;
}

void Info::_Scope::dump(int indent) const {
#ifdef MY_DEBUG
    qDebug().noquote() << QString(indent, ' ') << _M_entry;
    for (const auto &child : _M_children) {
        qDebug().noquote().nospace() << QString(indent, ' ')
                                     << '[' << child.first << ',' << child.second.first << ')';
        child.second.second.dump(indent + 1);
    }
#endif
}

const Info::_Scope Info::_Scope::_S_null;

void Info::__parse(Line &line, LocLists &loclists, RngLists &rnglists) {
    struct Fixup {
        _Word entry;
        _At at;
        _Form form;
    };
    struct Unit {
        _Off offset;
        std::vector<Fixup> fixups;
        std::vector<_Word> file_map;
        std::vector<std::pair<_Word, _Word>> loclists, rnglists;
    };
    std::vector<Unit> units;
    std::unordered_map<_Off, _Word> entry_offsets;

    {
        std::vector<_Word> stack{ _InvalidWord };
        Data data = _M_data;
        while (!data.empty()) {
            _Off unit_offset = std::distance(_M_data.begin(), data.begin());
            auto unit_length = consume<_Word>(data);
            auto unit = data.take_first(unit_length);
            auto version = consume<_Half>(unit);
            if (version < 2 || version > 5)
                return error("Unsupported info version");
            auto unit_type = version < 5 ? _Byte(DW_UT_compile) : consume<_Byte>(unit);
            if (unit_type != DW_UT_compile)
                return error("Unsupported unit type");
            _Byte address_size;
            if (version >= 5) {
                address_size = consume<_Byte>(unit);
            }
            auto abbrevs = _M_file.abbrev().get(consume<_Off>(unit));
            if (version < 5) {
                address_size = consume<_Byte>(unit);
            }
            if (address_size != sizeof(_Addr))
                return error("Unsupported address size");

            units.emplace_back();
            units.back().offset = unit_offset;
            auto first = true;
            Abbrev::_Attr early_fixups[] = {
                { _At::DW_AT_name,     _Form::_None, {} },
                { _At::DW_AT_comp_dir, _Form::_None, {} },
                { _At::DW_AT_low_pc,   _Form::_None, {} },
            };
            while (!error() && !unit.empty()) {
                _Off entry_offset = std::distance(_M_data.begin(), unit.begin());
                auto code = consume_uleb128(unit);
                if (code) {
                    auto abbrev = abbrevs.find(code);
                    if (abbrev == abbrevs.end())
                        return error("Missing abbrev code");
                    if (first != (abbrev->second._M_tag == _Tag::DW_TAG_compile_unit))
                        return error("Expected only the first entry to be a compile unit");

                    _Word entry_index = _M_entries.size();
                    entry_offsets.emplace(entry_offset, entry_index);
                    if (stack.back() != _InvalidWord) {
                        _M_entries[stack.back()]._M_sibling = entry_index;
                    }
                    stack.back() = entry_index;
                    if (abbrev->second._M_children) {
                        stack.push_back(_InvalidWord);
                    }

                    _M_entries.emplace_back();
                    auto &entry = _M_entries.back();
                    entry._M_tag = abbrev->second._M_tag;
                    entry._M_children = abbrev->second._M_children;
                    for (const auto &attr : abbrev->second._M_attrs) {
                        auto fixup = _Form::_None;
                        auto val = consume_form(attr, unit, fixup);
                        if (attr._M_at == _At::_Unknown) {
                            continue;
                        }
                        auto inserted = entry._M_attrs.emplace(attr._M_at, val);
                        if (!inserted.second)
                            return error("Duplicate attr");
                        else if (val.is_none()) {
#ifdef MY_DEBUG
                            qWarning() << "Unsupported form" << attr._M_form;
#endif
                            return error("Unsupported form");
                        }
                        if (fixup != _Form::_None) {
                            if (first) {
                                for (auto &early_fixup : early_fixups) {
                                    if (attr._M_at == early_fixup._M_at) {
                                        early_fixup._M_form = std::exchange(fixup, _Form::_None);
                                        break;
                                    }
                                }
                            }
                            if (fixup != _Form::_None) {
                                units.back().fixups.push_back({ entry_index, attr._M_at, fixup });
                            }
                        }
                    }

                    if (first) {
                        for (auto &early_fixup : early_fixups) {
                            if (early_fixup._M_form == _Form::_None) {
                                continue;
                            }
                            auto &val = entry._M_attrs[early_fixup._M_at];
                            switch (early_fixup._M_form) {
                                case _Form::DW_FORM_strp:
                                    val = _M_file.str().get(val.cst());
                                    break;
                                case _Form::DW_FORM_strx:
                                case _Form::DW_FORM_strx1:
                                case _Form::DW_FORM_strx2:
                                case _Form::DW_FORM_strx3:
                                case _Form::DW_FORM_strx4:
                                    val = strx(val.cst(), entry, _M_file);
                                    break;
                                case _Form::DW_FORM_addrx:
                                case _Form::DW_FORM_addrx1:
                                case _Form::DW_FORM_addrx2:
                                case _Form::DW_FORM_addrx3:
                                case _Form::DW_FORM_addrx4:
                                    val = addrx(val.cst(), entry, _M_file);
                                    break;
                                default:
#ifdef MY_DEBUG
                                    qWarning() << "Unsupported early form" << early_fixup._M_form;
#endif
                                    return error("Unsupported early form");
                            }
                        }
                        units.back().file_map = line.__parse(address_size, entry);
                        units.back().loclists = loclists.__parse(entry);
                        units.back().rnglists = rnglists.__parse(entry);
                    }
                    first = false;
                } else if (stack.size() > 1u) {
                    stack.pop_back();
                }
            }
            if (stack.size() != 1u)
                return error("Unexpected end of info unit");

            if (error()) return;
        }
        _M_entries.shrink_to_fit();
        line.shrink_to_fit();
        loclists.shrink_to_fit();
        rnglists.shrink_to_fit();
    }

    {
        _Word unit_index{};
        for (const auto &unit : units) {
            const auto &unit_entry = _M_entries[unit_index];
            auto addr = _M_file.addr().get(unit_entry);
            auto str_offsets = _M_file.str_offsets().get(unit_entry);

            for (const auto &fixup : unit.fixups) {
                if (error()) return;

                auto &val = _M_entries[fixup.entry]._M_attrs.at(fixup.at);
                switch (fixup.form) {
                    case _Form::DW_FORM_strp:
                        val = _M_file.str().get(val.cst());
                        break;
                    case _Form::DW_FORM_ref_addr:
                    case _Form::DW_FORM_ref1:
                    case _Form::DW_FORM_ref2:
                    case _Form::DW_FORM_ref4:
                    case _Form::DW_FORM_ref8:
                    case _Form::DW_FORM_ref_udata: {
                        auto base = fixup.form == _Form::DW_FORM_ref_addr ? 0u : unit.offset;
                        auto offset = entry_offsets.find(base + val.cst());
                        if (offset == entry_offsets.end())
                            return error("Unknown entry offset");
                        val = _M_entries[offset->second];
                        break;
                    }
                    case _Form::DW_FORM_strx:
                    case _Form::DW_FORM_strx1:
                    case _Form::DW_FORM_strx2:
                    case _Form::DW_FORM_strx3:
                    case _Form::DW_FORM_strx4:
                        if (val.cst() >= str_offsets.size())
                            return error("Out of range index into str offsets");
                        val = _M_file.str().get(str_offsets[val.cst()]);
                        break;
                    case _Form::DW_FORM_addrx:
                    case _Form::DW_FORM_addrx1:
                    case _Form::DW_FORM_addrx2:
                    case _Form::DW_FORM_addrx3:
                    case _Form::DW_FORM_addrx4:
                        if (val.cst() >= addr.size())
                            return error("Out of range index into addr");
                        val = addr[val.cst()];
                        break;
                    case _Form::DW_FORM_line_strp:
                        val = _M_file.line_str().get(val.cst());
                        break;
                    case _Form::DW_FORM_loclistx:
                        if (val.cst() >= unit.loclists.size())
                            return error("Out of range index into location lists");
                        val = _M_file.loclists().get(unit.loclists[val.cst()]);
                        break;
                    case _Form::DW_FORM_rnglistx:
                        if (val.cst() >= unit.rnglists.size())
                            return error("Out of range index into range lists is");
                        val = _M_file.rnglists().get(unit.rnglists[val.cst()]);
                        break;
                    case _Form::_HighPC:
                        if (val.is_cst()) {
                            val = _M_entries[fixup.entry].attr(_At::DW_AT_low_pc).addr() + _Addr(val.cst());
                        }
                        break;
                    case _Form::_DeclFile:
                        if (val.cst() >= unit.file_map.size())
                            return error("Out of range index into files");
                        val = _M_file.line().get(unit.file_map[val.cst()]);
                        break;
                    default:
                        return error("Unknown fixup");
                }
            }
            unit_index = _M_entries[unit_index]._M_sibling;
        }
    }

    {
        std::vector<std::pair<std::reference_wrapper<const _Die>,
                              std::reference_wrapper<_Scope>>> stack{
            { _Die::_S_null, _M_scopes.front() }
        };
        bool children = true;
        for (const _Die &entry : _M_entries) {
            if (!children) {
                bool sibling = false;
                while (!sibling) {
                    assert(stack.size() > 1 && "Invalid tree structure");
                    sibling = stack.back().first.get()._M_sibling;
                    stack.pop_back();
                }
            }
            children = entry._M_children;
            switch (entry._M_tag) {
                case _Tag::DW_TAG_compile_unit:
                    break;
                case _Tag::DW_TAG_subprogram:
                    if (!(entry.attr(_At::DW_AT_declaration).flag() ||
                          entry.attr(_At::DW_AT_inline).cst() & 1))
                        break;
                    stack.emplace_back(entry, _M_scopes.front());
                    continue;
                case _Tag::DW_TAG_lexical_block:
                case _Tag::DW_TAG_inlined_subroutine:
                    if (stack.back().second.get().entry()._M_tag != _Tag::_None)
                        break;
                    [[gnu::fallthrough]];
                default:
                    switch (entry._M_tag) {
                        case _Tag::DW_TAG_label:
                        case _Tag::DW_TAG_variable:
                            stack.back().second.get().add(entry);
                            break;
                        default:
                            break;
                    }
                    stack.emplace_back(entry, stack.back().second);
                    continue;
            }
            _M_scopes.emplace_back(_M_scopes.back(), entry);
            const auto &low_pc = entry.attr(_At::DW_AT_low_pc);
            const auto &high_pc = entry.attr(_At::DW_AT_high_pc);
            const auto &ranges = entry.attr(_At::DW_AT_ranges);
            if (low_pc.is_addr() && high_pc.is_addr())
                stack.back().second.get().set(low_pc.addr(), high_pc.addr(), _M_scopes.back());
            else if (ranges.is_rnglist())
                for (const auto &rng : ranges.rnglist())
                    stack.back().second.get().set(rng.first, rng.second, _M_scopes.back());
            stack.emplace_back(entry, _M_scopes.back());
        }
    }

#ifdef MY_DEBUG_DISABLED
    root().dump();
#endif

#ifdef MY_DEBUG_DISABLED
    if (!error()) {
        for (std::size_t i{}; i != _M_file.line().size(); ++i) {
            const auto &file = _M_file.line().get(i);
            auto debug = qDebug() << file.path() << ':';
            for (auto part : file.path())
                debug << part;
        }
        qDebug() << "";
    }
#endif

#ifdef MY_DEBUG_DISABLED
    if (!error()) {
        for (const auto &entry : _M_entries) {
            switch (entry._M_tag) {
                default: break; // whatever
                case _Tag::DW_TAG_compile_unit:
                    qDebug() << entry._M_tag << ':' << entry._M_attrs.at(_At::DW_AT_low_pc)
                             << '-' << entry._M_attrs.at(_At::DW_AT_high_pc);
                    break;
            }
        }
    }
#endif

#ifdef MY_DEBUG_DISABLED
    if (!error()) {
        std::vector<bool> stack;
        QString indent;
        for (const auto &entry : _M_entries) {
            qDebug().noquote().nospace() << indent << entry._M_tag;
            for (const auto &attr : entry._M_attrs)
                qDebug().noquote().nospace() << indent << "    " << attr.first
                                             << '(' << attr.second << ')';
            if (entry._M_children) {
                indent += "  ";
                stack.push_back(entry._M_sibling);
            } else if (!entry._M_sibling && !stack.empty()) {
                bool cur;
                do {
                    indent.chop(2);
                    cur = stack.back();
                    stack.pop_back();
                } while (!cur && !stack.empty());
            }
        }
    }
#endif
}

std::vector<_Word> Line::__parse(_Byte address_size, const _Die &unit_entry) {
    std::vector<_Word> file_map;
    auto base = unit_entry.attr(_At::DW_AT_stmt_list);
    if (base.is_none()) {
        return file_map;
    }
    auto unit = _M_data.subview(base.cst());
    auto unit_length = consume<_Word>(unit);
    unit = unit.first(unit_length);
    auto version = consume<_Half>(unit);
    if (version < 2 || version > 5) {
        error("Unsupported line version");
        return {};
    }
    _Byte segment_selector_size{};
    if (version >= 5) {
        address_size = consume<_Byte>(unit);
        segment_selector_size = consume<_Byte>(unit);
    }
    if (address_size != sizeof(_Addr)) {
        error("Unsupported address size");
        return {};
    }
    if (segment_selector_size) {
        error("Unsupported segment selector size");
        return {};
    }
    auto header_length = consume<_Word>(unit);
    auto header = unit.take_first(header_length);
    auto minimum_instruction_length = consume<_Byte>(header);
    if (minimum_instruction_length != 1) {
        error("Unsupported minimum instruction length");
        return {};
    }
    auto maximum_operations_per_instruction = version < 4 ? 1u : consume<_Byte>(header);
    if (maximum_operations_per_instruction != 1) {
        error("Unsupported maximum operations per instruction");
        return {};
    }
    auto default_is_stmt = consume<_Byte>(header);
    auto line_base = consume<_Sbyte>(header);
    auto line_range = consume<_Byte>(header);
    auto opcode_base = consume<_Byte>(header);
    if (!opcode_base) {
        error("Invalid opcode base");
        return {};
    }
    auto standard_opcode_lengths = header.take_first(opcode_base - 1).bitcast<const _Byte>();
    auto parse_header_entries = [&](std::initializer_list<_EntryFormat> default_format,
                                    _At default_path_attr) -> std::vector<_Entry> {
        auto entry_format_count = version < 5 ? default_format.size() : consume<_Byte>(header);
        std::array<_EntryFormat, 5> entry_format;
        if (entry_format_count > entry_format.size()) {
            error("Unsupported entry format count");
            return {};
        }
        if (version < 5) {
            std::copy_n(default_format.begin(), entry_format_count, entry_format.begin());
        } else {
            for (decltype(entry_format_count) i{}; i != entry_format_count; i++) {
                entry_format[i].first = _Lnct(consume_uleb128(header).get());
                entry_format[i].second = _Form(consume_uleb128(header).get());
            }
        }
        auto entries_count = version < 5 ? 0u : consume_uleb128(header).get();
        std::vector<_Entry> entries;
        entries.reserve(entries_count);
        if (version < 5) {
            entries.emplace_back();
            entries.back()._M_path = unit_entry.attr(default_path_attr).str();
            entries.back()._M_directory_index = 0;
        }
        while (version < 5 || entries.size() < entries_count) {
            if (header.empty()) {
                error("Unexpected end of header");
                return {};
            }
            _Entry entry;
            for (decltype(entry_format_count) i{}; i != entry_format_count; i++) {
                auto fixup = _Form::_Unused;
                auto val = consume_form({ _At::_None, entry_format[i].second, 0 }, header, fixup);
                if (fixup != _Form::_Unused) {
                    error("Unexpected fixup in line header");
                    return {};
                }
                switch (entry_format[i].first) {
                    case _Lnct::DW_LNCT_path:
                        if (!val.is_str()) {
                            error("Unexpected form for path");
                            return {};
                        }
                        if (version < 5 && val.str() == "") {
                            return entries;
                        }
                        entry._M_path = val.str();
                        break;
                    case _Lnct::DW_LNCT_directory_index:
                        if (!val.is_cst()) {
                            error("Unexpected form for directory index");
                            return {};
                        }
                        entry._M_directory_index = val.cst();
                        break;
                    case _Lnct::DW_LNCT_timestamp:
                        if (!val.is_cst() && !val.is_data()) {
                            error("Unexpected form for timestamp");
                            return {};
                        }
                        entry._M_timestamp = val.cst();
                        break;
                    case _Lnct::DW_LNCT_size:
                        if (!val.is_cst()) {
                            error("Unexpected form for size");
                            return {};
                        }
                        entry._M_size = val.cst();
                        break;
                    case _Lnct::DW_LNCT_MD5:
                        if (!val.is_data()) {
                            error("Unexpected form for MD5");
                            return {};
                        }
                        if (val.data().size_bytes() != 16) {
                            error("Unexpected size for MD5");
                            return {};
                        }
                        entry._M_MD5 = val.data();
                        break;
                    default:
                        error("Unsupported line number content description");
                        return {};
                }
            }
            entries.emplace_back(std::move(entry));
        }
        return entries;
    };
    auto directories = parse_header_entries({
            { _Lnct::DW_LNCT_path,            _Form::DW_FORM_string },
        }, _At::DW_AT_comp_dir);
    auto file_names = parse_header_entries({
            { _Lnct::DW_LNCT_path,            _Form::DW_FORM_string },
            { _Lnct::DW_LNCT_directory_index, _Form::DW_FORM_udata  },
            { _Lnct::DW_LNCT_timestamp,       _Form::DW_FORM_udata  },
            { _Lnct::DW_LNCT_size,            _Form::DW_FORM_udata  },
        }, _At::DW_AT_name);
    file_map.reserve(file_names.size());
    for (const auto &file : file_names) {
        _Str dir;
        if (file._M_directory_index < directories.size()) {
            dir = directories[file._M_directory_index]._M_path;
        }
        auto inserted = _M_paths.insert({ { unit_entry.attr(_At::DW_AT_comp_dir).str(),
                                            dir, file._M_path }, size() });
        if (inserted.second) {
            _M_files.emplace_back(inserted.first->first, file._M_MD5.data());
        }
        file_map.push_back(inserted.first->second);
    }

    _Loc state;
    auto reset_state = [&]() {
        state._M_file = 1;
        state._M_line = 1;
        state._M_column = 0;
        state._M_address = 0;
        state._M_is_stmt = default_is_stmt;
        state._M_basic_block = false;
        state._M_end_sequence = false;
        state._M_prologue_end = false;
        state._M_epilogue_begin = false;
        state._M_isa = 0;
        state._M_discriminator = 0;
    };
    reset_state();
    auto emit_loc = [&]() {
        if (state._M_file >= file_map.size())
            return error("Out of bounds file index");
        auto file = file_map[state._M_file];
#ifdef MY_DEBUG_DISABLED
        qDebug() << _M_files[file].path() << state._M_line <<  state._M_column
                 << Qt::showbase << Qt::hex << _Word(state._M_address);
#endif
        auto inserted_loc = _M_locs.emplace(state._M_address, state);
        if (!inserted_loc.second &&
            inserted_loc.first->second._M_end_sequence) {
            inserted_loc.first->second = state;
            inserted_loc.second = true;
        }
        if (inserted_loc.second) {
            inserted_loc.first->second._M_file = file;
            if (state._M_is_stmt) {
                auto inserted_line = _M_files[file].lines().insert(
                        { { state._M_line, state._M_column }, state._M_address });
                if (!inserted_line.second &&
                    inserted_line.first->second > state._M_address) {
                    inserted_line.first->second = state._M_address;
                }
            }
        }
        state._M_basic_block = false;
        state._M_prologue_end = false;
        state._M_epilogue_begin = false;
        state._M_discriminator = 0;
    };

    while (!error() && !unit.empty()) {
        auto opc = consume<_Byte>(unit);
        switch (opc) {
            default:
                if (opc >= opcode_base) {
                    // 1. special opcodes
                    opc -= opcode_base;
                    state._M_address += opc / line_range
                        / maximum_operations_per_instruction
                        * minimum_instruction_length;
                    state._M_line += line_base + opc % line_range;
                    emit_loc();
                } else {
                    // 2. standard opcodes
                    for (auto i = standard_opcode_lengths[opc - 1]; i; --i) {
                        (void)consume_uleb128(unit);
                    }
                }
                break;
            case DW_LNS_copy:
                emit_loc();
                break;
            case DW_LNS_advance_pc:
                state._M_address += consume_uleb128(unit).get();
                break;
            case DW_LNS_advance_line:
                state._M_line += consume_sleb128(unit);
                break;
            case DW_LNS_set_file:
                state._M_file = consume_uleb128(unit);
                break;
            case DW_LNS_set_column:
                state._M_column = consume_uleb128(unit);
                break;
            case DW_LNS_negate_stmt:
                state._M_is_stmt = !state._M_is_stmt;
                break;
            case DW_LNS_set_basic_block:
                state._M_basic_block = true;
                break;
            case DW_LNS_const_add_pc:
                state._M_address += (0xFFu - opcode_base) / line_range
                    / maximum_operations_per_instruction * minimum_instruction_length;
                break;
            case DW_LNS_fixed_advance_pc:
                state._M_address += consume<_Half>(unit);
                break;
            case DW_LNS_set_prologue_end:
                state._M_prologue_end = true;
                break;
            case DW_LNS_set_epilogue_begin:
                state._M_epilogue_begin = true;
                break;
            case DW_LNS_set_isa:
                state._M_isa = consume_uleb128(unit);
                break;
            case DW_LNS_extended: {
                auto len = consume_uleb128(unit);
                auto instruction = unit.take_first(len);
                switch (opc = consume<_Byte>(instruction)) {
                    // 3. extended opcodes
                    case DW_LNE_end_sequence:
                        state._M_end_sequence = true;
                        emit_loc();
                        reset_state();
                        break;
                    case DW_LNE_set_address: {
                        state._M_address = consume<_Addr>(instruction);
                        break;
                    }
                    case DW_LNE_set_discriminator:
                        state._M_discriminator = consume_uleb128(instruction);
                        break;
                }
                break;
            }
        }
    }
    return file_map;
}
const SrcFile &Line::get(_Files::size_type pos) const noexcept {
    if (pos >= size()) {
        error("Index into files out of bounds");
        return SrcFile::_S_null;
    }
    return _M_files[pos];
}

std::vector<std::pair<_Word, _Word>> LocLists::__parse(const _Die &unit_entry) {
    std::vector<std::pair<_Word, _Word>> indices;
    auto base = unit_entry.attr(_At::DW_AT_loclists_base);
    if (base.is_none()) {
        return indices;
    }
    auto unit = _M_data.subview(base.cst() - sizeof(_Word)
                                - sizeof(_Half) - sizeof(_Byte) - sizeof(_Byte) - sizeof(_Word));
    auto unit_length = consume<_Word>(unit);
    unit = unit.first(unit_length);
    auto version = consume<_Half>(unit);
    if (version < 5 || version > 5) {
        error("Unsupported loclists version");
        return {};
    }
    auto address_size = consume<_Byte>(unit);
    if (address_size != sizeof(_Addr)) {
        error("Unsupported address size");
        return {};
    }
    auto segment_selector_size = consume<_Byte>(unit);
    if (segment_selector_size) {
        error("Unsupported segment selector size");
        return {};
    }
    auto offset_entry_count = consume<_Word>(unit);
    auto addr = _M_file.addr().get(unit_entry);
    auto base_addr = unit_entry.attr(_At::DW_AT_low_pc).addr();
    for (auto offset : unit.bitcast<const _Off>().first(offset_entry_count)) {
        if (error()) {
            return {};
        }

        indices.emplace_back(_M_locs.size(), _InvalidWord);
        auto loclist = unit.subview(offset);
        while (!error() && indices.back().second == _InvalidWord) {
            _Long base_index, start_index, end_index;
            _Addr start_addr, end_addr;
            switch (consume<_Byte>(loclist)) {
                case DW_LLE_end_of_list:
                    indices.back().second = _M_locs.size();
                    continue;
                case DW_LLE_base_addressx:
                    base_index = consume_uleb128(loclist);
                    if (base_index >= addr.size()) {
                        error("Out of range index into addr");
                        return {};
                    }
                    base_addr = addr[base_index];
                    continue;
                case DW_LLE_startx_endx:
                    start_index = consume_uleb128(loclist);
                    end_index = consume_uleb128(loclist);
                    if (start_index >= addr.size() ||
                        end_index >= addr.size()) {
                        error("Out of range index into addr");
                        return {};
                    }
                    start_addr = addr[start_index];
                    end_addr = addr[end_index];
                    break;
                case DW_LLE_startx_length:
                    start_index = consume_uleb128(loclist);
                    if (start_index >= addr.size()) {
                        error("Out of range index into addr");
                        return {};
                    }
                    start_addr = addr[start_index];
                    end_addr = start_addr + _Addr(consume_uleb128(loclist));
                    break;
                case DW_LLE_offset_pair:
                    start_addr = base_addr + _Addr(consume_uleb128(loclist));
                    end_addr = base_addr + _Addr(consume_uleb128(loclist));
                    break;
                case DW_LLE_default_location:
                    _M_locs.push_back({ { std::numeric_limits<_Addr>::min(),
                                          std::numeric_limits<_Addr>::max() }, {} });
                    continue;
                case DW_LLE_base_address:
                    base_addr = consume<_Addr>(loclist);
                    continue;
                case DW_LLE_start_end:
                    start_addr = consume<_Addr>(loclist);
                    end_addr = consume<_Addr>(loclist);
                    break;
                case DW_LLE_start_length:
                    start_addr = consume<_Addr>(loclist);
                    end_addr = start_addr + consume<_Addr>(loclist);
                    break;
                default:
                    error("Unknown range list entry kind");
                    return {};
            }
            _M_locs.push_back({ { start_addr, end_addr },
                                loclist.take_first(consume_uleb128(loclist)) });
        }
    }
    return indices;
}
_LocView LocLists::get(std::pair<_Word, _Word> indices) const noexcept {
    if (indices.first > indices.second || indices.first > _M_locs.size()
                                       || indices.second > _M_locs.size()) {
        error("Out of range index into location offsets");
        return {};
    }
    return { _M_locs.data() + indices.first, _M_locs.data() + indices.second };
}

std::vector<std::pair<_Word, _Word>> RngLists::__parse(const _Die &unit_entry) {
    std::vector<std::pair<_Word, _Word>> indices;
    auto base = unit_entry.attr(_At::DW_AT_rnglists_base);
    if (base.is_none()) {
        return indices;
    }
    auto unit = _M_data.subview(base.cst() - sizeof(_Word)
                                - sizeof(_Half) - sizeof(_Byte) - sizeof(_Byte) - sizeof(_Word));
    auto unit_length = consume<_Word>(unit);
    unit = unit.first(unit_length);
    auto version = consume<_Half>(unit);
    if (version < 5 || version > 5) {
        error("Unsupported rnglists version");
        return {};
    }
    auto address_size = consume<_Byte>(unit);
    if (address_size != sizeof(_Addr)) {
        error("Unsupported address size");
        return {};
    }
    auto segment_selector_size = consume<_Byte>(unit);
    if (segment_selector_size) {
        error("Unsupported segment selector size");
        return {};
    }
    auto offset_entry_count = consume<_Word>(unit);
    auto addr = _M_file.addr().get(unit_entry);
    auto base_addr = unit_entry.attr(_At::DW_AT_low_pc).addr();
    for (auto offset : unit.bitcast<const _Off>().first(offset_entry_count)) {
        if (error()) {
            return {};
        }

        indices.emplace_back(_M_rngs.size(), _InvalidWord);
        auto rnglist = unit.subview(offset);
        while (!error() && indices.back().second == _InvalidWord) {
            _Long base_index, start_index, end_index;
            _Addr start_addr, end_addr;
            switch (consume<_Byte>(rnglist)) {
                case DW_RLE_end_of_list:
                    indices.back().second = _M_rngs.size();
                    continue;
                case DW_RLE_base_addressx:
                    base_index = consume_uleb128(rnglist);
                    if (base_index >= addr.size()) {
                        error("Out of range index into addr");
                        return {};
                    }
                    base_addr = addr[base_index];
                    continue;
                case DW_RLE_startx_endx:
                    start_index = consume_uleb128(rnglist);
                    end_index = consume_uleb128(rnglist);
                    if (start_index >= addr.size() ||
                        end_index >= addr.size()) {
                        error("Out of range index into addr");
                        return {};
                    }
                    start_addr = addr[start_index];
                    end_addr = addr[end_index];
                    break;
                case DW_RLE_startx_length:
                    start_index = consume_uleb128(rnglist);
                    if (start_index >= addr.size()) {
                        error("Out of range index into addr");
                        return {};
                    }
                    start_addr = addr[start_index];
                    end_addr = start_addr + _Addr(consume_uleb128(rnglist));
                    break;
                case DW_RLE_offset_pair:
                    start_addr = base_addr + _Addr(consume_uleb128(rnglist));
                    end_addr = base_addr + _Addr(consume_uleb128(rnglist));
                    break;
                case DW_RLE_base_address:
                    base_addr = consume<_Addr>(rnglist);
                    continue;
                case DW_RLE_start_end:
                    start_addr = consume<_Addr>(rnglist);
                    end_addr = consume<_Addr>(rnglist);
                    break;
                case DW_RLE_start_length:
                    start_addr = consume<_Addr>(rnglist);
                    end_addr = start_addr + consume<_Addr>(rnglist);
                    break;
                default:
                    error("Unknown range list entry kind");
                    return {};
            }
            _M_rngs.emplace_back(start_addr, end_addr);
        }
    }
    return indices;
}
_RngView RngLists::get(std::pair<_Word, _Word> indices) const noexcept {
    if (indices.first > indices.second || indices.first > _M_rngs.size()
                                       || indices.second > _M_rngs.size()) {
        error("Out of range index into range offsets");
        return {};
    }
    return { _M_rngs.data() + indices.first, _M_rngs.data() + indices.second };
}

_Str Str::get(_Word offset) const noexcept {
    Data temp(_M_data.subview(offset));
    return consume_strz(temp);
}

_OffView StrOffsets::get(const _Die &unit_entry) const noexcept {
    auto base = unit_entry.attr(_At::DW_AT_str_offsets_base);
    if (base.is_none()) {
        return {};
    }
    auto unit = _M_data.subview(base.cst() - sizeof(_Word) - sizeof(_Half) - sizeof(_Half));
    unit = unit.first(consume<_Word>(unit));
    auto version = consume<_Half>(unit);
    if (version < 5 || version > 5) {
        error("Unsupported str offsets version");
        return {};
    }
    consume<_Half>(unit);
    return unit.bitcast<_OffView::element_type>();
}
} // end namespace section
} // end namespace dwarf
} // end namespace debuginfo
