#include "vartablemodel.h"

#include "../../core/vat.h"
#include "utils.h"

#include <QtGui/QBrush>

Q_DECLARE_METATYPE(calc_var_t)

VarTableModel::VarData::VarData(const calc_var_t &var) : info(var), data(reinterpret_cast<char*>(var.data), var.size), checked(Qt::Unchecked) {
    info.data = reinterpret_cast<uint8_t*>(data.data());
    updatePreview();
}

void VarTableModel::VarData::updatePreview() {
    previewValid = true;
    if (info.size <= 2) {
        preview = tr("Empty");
        previewValid = false;
    } else if (calc_var_is_asmprog(&info)) {
        preview = tr("Can't preview this");
        previewValid = false;
    } else if (calc_var_is_internal(&info) && info.name[0] != '#') { // # is previewable
        preview = tr("Can't preview this OS variable");
        previewValid = false;
    } else {
        try {
            preview = QString::fromStdString(calc_var_content_string(info)).trimmed().replace("\n", " \\ ");
            if (preview.size() > 50) {
                preview.truncate(50);
                preview += QStringLiteral(" [...]");
            }
        } catch (...) {
            preview = tr("Can't preview this");
            previewValid = false;
        }
    }
}

VarTableModel::VarTableModel(QObject *parent) : QAbstractTableModel(parent) {
    varPreviewCEFont = QFont(QStringLiteral("TICELarge"), 11);
    varPreviewItalicFont.setItalic(true);
}

void VarTableModel::clear() {
    beginResetModel();
    vars.clear();
    endResetModel();
}

void VarTableModel::refresh() {
    calc_var_t var;

    clear();

    vat_search_init(&var);
    while (vat_search_next(&var)) {
        if (var.named || var.size > 2) {
            int row = vars.size();
            beginInsertRows(QModelIndex(), row, row);
            vars.push_back(VarData(var));
            endInsertRows();
        }
    }
}

void VarTableModel::retranslate() {
    if (!vars.empty()) {
        for (VarData &var : vars) {
            var.updatePreview();
        }
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
    emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
}

QVariant VarTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    const VarData &var = vars[index.row()];
    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case VAR_NAME_COL:
                    return QString::fromUtf8(calc_var_name_to_utf8(var.info.name, var.info.namelen, var.info.named));
                case VAR_LOCATION_COL:
                    return var.info.archived ? tr("Archive") : QStringLiteral("RAM");
                case VAR_TYPE_COL:
                {
                    // Do not translate - things rely on those names.
                    QString var_type_str = calc_var_type_names[var.info.type];
                    if (calc_var_is_asmprog(&var.info)) {
                        var_type_str += QStringLiteral(" (ASM)");
                    }
                    return var_type_str;
                }
                case VAR_SIZE_COL:
                    return var.info.size;
                case VAR_PREVIEW_COL:
                    return var.preview;
                default:
                    return QVariant();
            };
        case Qt::FontRole:
            if (index.column() == VAR_PREVIEW_COL) {
                return var.previewValid ? varPreviewCEFont : varPreviewItalicFont;
            }
            return QVariant();
        case Qt::ForegroundRole:
            if (index.column() == VAR_PREVIEW_COL && !var.previewValid) {
                return QBrush(Qt::gray);
            }
            return QVariant();
        case Qt::CheckStateRole:
            if (index.column() == VAR_NAME_COL) {
                return var.checked;
            }
            return QVariant();
        case Qt::UserRole:
            return QVariant::fromValue(var.info);
        default:
            return QVariant();
    };
}

bool VarTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (index.isValid() && index.column() == VAR_NAME_COL && role == Qt::CheckStateRole) {
        vars[index.row()].checked = qvariant_cast<Qt::CheckState>(value);
        emit dataChanged(index, index, { Qt::CheckStateRole });
        return true;
    }
    return false;
}

QVariant VarTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case VAR_NAME_COL:
                return tr("Name");
            case VAR_LOCATION_COL:
                return tr("Location");
            case VAR_TYPE_COL:
                return tr("Type");
            case VAR_SIZE_COL:
                return tr("Size");
            case VAR_PREVIEW_COL:
                return tr("Preview");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags VarTableModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if (index.column() == VAR_NAME_COL) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

int VarTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return vars.size();
}

int VarTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return VAR_NUM_COLS;
}