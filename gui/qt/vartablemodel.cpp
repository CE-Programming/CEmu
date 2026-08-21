#include "vartablemodel.h"

#include "../../core/vat.h"
#include "utils.h"

#include <QtGui/QBrush>
#include <QtCore/QSet>
#include <algorithm>
#include <cassert>

Q_DECLARE_METATYPE(calc_var_t)

VarTableModel::VarData::VarData(const calc_var_t &var)
    : info(var), previewState(PreviewState::Outdated), checked(false) {
    info.data = new uint8_t[var.size];
    memcpy(info.data, var.data, var.size);
}

VarTableModel::VarData::~VarData() {
    delete[] info.data;
}

VarTableModel::VarData::VarData(VarData &&other) noexcept {
    info.data = nullptr;
    *this = std::move(other);
}

VarTableModel::VarData &VarTableModel::VarData::operator=(VarData &&other) noexcept {
    uint8_t *data = info.data;
    info = other.info;
    other.info.data = data;
    preview = std::move(other.preview);
    previewState = other.previewState;
    checked = other.checked;
    return *this;
}

uint8_t VarTableModel::VarData::updateInfo(const calc_var_t &var) {
    uint8_t changed = 0;
    if (var.namelen != info.namelen ||
        0 != memcmp(var.name, info.name, var.namelen)) {
        changed |= (1 << VAR_NAME_COL);
    }
    if (var.archived != info.archived) {
        changed |= (1 << VAR_LOCATION_COL);
    }
    if (var.type != info.type) {
        changed |= (1 << VAR_TYPE_COL);
    }
    uint8_t *data = info.data;
    if (var.size != info.size) {
        changed |= (1 << VAR_TYPE_COL) | (1 << VAR_SIZE_COL) |
                   (1 << VAR_PREVIEW_COL);
        data = new uint8_t[var.size];
    } else if (0 != memcmp(var.data, data, var.size)) {
        changed |= (1 << VAR_TYPE_COL) | (1 << VAR_PREVIEW_COL);
    }
    if (data != info.data) {
        delete[] info.data;
    }
    info = var;
    info.data = data;
    if (changed & (1 << VAR_PREVIEW_COL)) {
        memcpy(info.data, var.data, var.size);
        previewState = PreviewState::Outdated;
    }
    return changed;
}

void VarTableModel::VarData::updatePreview() {
    previewState = PreviewState::Valid;
    if (info.size <= 2) {
        preview = tr("Empty");
        previewState = PreviewState::Invalid;
    } else if (calc_var_is_asmprog(&info)) {
        preview = tr("Can't preview this");
        previewState = PreviewState::Invalid;
    } else if (calc_var_is_internal(&info) && info.name[0] != '#') { // # is previewable
        preview = tr("Can't preview this OS variable");
        previewState = PreviewState::Invalid;
    } else {
        try {
            preview = QString::fromStdString(calc_var_content_string(info)).trimmed().replace("\n", " \\ ");
            if (preview.size() > 50) {
                preview.truncate(50);
                preview += QStringLiteral(" [...]");
            }
        } catch (...) {
            preview = tr("Can't preview this");
            previewState = PreviewState::Invalid;
        }
    }
}

VarTableModel::VarTableModel(QObject *parent) : QAbstractTableModel(parent) {
    varPreviewCEFont = QFont(QStringLiteral("TICELarge"), 11);
    varPreviewItalicFont.setItalic(true);
}

QByteArray VarTableModel::variableKey(const calc_var_t &var) {
    QByteArray key;
    key.reserve(11);
    key.append(static_cast<char>(calc_var_normalized_type(var.type)));
    key.append(static_cast<char>(var.named));
    const int nameLength = var.namelen - calc_var_is_list(&var);
    key.append(reinterpret_cast<const char *>(var.name), nameLength);
    return key;
}

QString VarTableModel::variableName(const calc_var_t &var) {
    return QString::fromUtf8(calc_var_name_to_utf8(var.name, var.namelen, var.named));
}

QString VarTableModel::configuredName(const QByteArray &key, const calc_var_t *var) const {
    const VariableConfig config = variableConfigs.value(key);
    const QString name = var ? variableName(*var) : config.name;
    const QString configuredAlias = config.alias;
    if (configuredAlias.isEmpty()) {
        return name;
    }
    return name.isEmpty() ? configuredAlias : tr("%1 (%2)").arg(configuredAlias, name);
}

void VarTableModel::clear() {
    beginResetModel();
    vars.clear();
    endResetModel();
}

QString VarTableModel::alias(const QModelIndex &index) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(vars.size())) {
        return QString();
    }
    return variableConfigs.value(variableKey(vars[index.row()].info)).alias;
}

void VarTableModel::setAlias(const QModelIndex &index, const QString &newAlias) {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(vars.size())) {
        return;
    }
    const QByteArray key = variableKey(vars[index.row()].info);
    VariableConfig &config = variableConfigs[key];
    config.alias = newAlias.trimmed();
    config.name = variableName(vars[index.row()].info);
    emit dataChanged(this->index(index.row(), VAR_NAME_COL),
                     this->index(index.row(), VAR_ALIAS_COL),
                     { Qt::DisplayRole, Qt::ToolTipRole });
    if (config.alias.isEmpty() && !config.watched) {
        variableConfigs.remove(key);
    }
}

bool VarTableModel::isWatched(const QModelIndex &index) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(vars.size())) {
        return false;
    }
    return variableConfigs.value(variableKey(vars[index.row()].info)).watched;
}

void VarTableModel::setWatched(const QModelIndex &index, bool watched) {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(vars.size())) {
        return;
    }
    const VarData &var = vars[index.row()];
    const QByteArray key = variableKey(var.info);
    VariableConfig &config = variableConfigs[key];
    config.name = variableName(var.info);
    config.watched = watched;
    config.watchInitialized = watched;
    config.present = watched;
    config.watchedData = watched
        ? QByteArray(reinterpret_cast<const char *>(var.info.data), var.info.size)
        : QByteArray();
    emit dataChanged(this->index(index.row(), VAR_NAME_COL),
                     this->index(index.row(), VAR_ALIAS_COL),
                     { Qt::FontRole, Qt::ToolTipRole });
    if (!watched && config.alias.isEmpty()) {
        variableConfigs.remove(key);
    }
    emit watchedVariablesChanged();
}

bool VarTableModel::hasWatchedVariables() const {
    for (const VariableConfig &config : variableConfigs) {
        if (config.watched) {
            return true;
        }
    }
    return false;
}

QStringList VarTableModel::checkWatchedVariables() {
    QStringList changedVariables;
    QSet<QByteArray> seen;
    calc_var_t var;
    vat_search_init(&var);
    while (vat_search_next(&var)) {
        const QByteArray key = variableKey(var);
        auto configIt = variableConfigs.find(key);
        if (configIt == variableConfigs.end() || !configIt->watched) {
            continue;
        }
        seen.insert(key);
        VariableConfig &config = configIt.value();
        config.name = variableName(var);
        const QByteArray data(reinterpret_cast<const char *>(var.data), var.size);
        if (config.watchInitialized && (!config.present || config.watchedData != data)) {
            changedVariables.append(configuredName(key, &var));
        }
        config.watchedData = data;
        config.watchInitialized = true;
        config.present = true;
    }

    for (auto configIt = variableConfigs.begin(); configIt != variableConfigs.end(); ++configIt) {
        VariableConfig &config = configIt.value();
        if (!config.watched || seen.contains(configIt.key())) {
            continue;
        }
        if (config.watchInitialized && config.present) {
            const QString name = configuredName(configIt.key());
            changedVariables.append(name.isEmpty() ? tr("Deleted variable") : tr("%1 (deleted)").arg(name));
        }
        config.watchedData.clear();
        config.watchInitialized = true;
        config.present = false;
    }
    return changedVariables;
}

static bool varLess(const calc_var_t &var, const calc_var_t &other) {
    return calc_var_compare_names(&var, &other) < 0;
}

void VarTableModel::refresh() {
    calc_var_t var;

    std::vector<calc_var_t> newVars;

    vat_search_init(&var);
    while (vat_search_next(&var)) {
        if (var.named || var.size > 2) {
            newVars.push_back(var);
        }
    }

    std::sort(newVars.begin(), newVars.end(), varLess);

    auto oldIter = vars.begin();
    auto newIter = newVars.begin();
    while (newIter != newVars.end()) {
        size_t row = oldIter - vars.begin();
        // Remove old vars smaller than the next new var
        auto oldRemoveEnd = std::find_if_not(oldIter, vars.end(), [=](const VarData &var) { return varLess(var.info, *newIter); });
        if (oldIter != oldRemoveEnd) {
            beginRemoveRows(QModelIndex(), row, row + (oldRemoveEnd - oldIter - 1));
            oldIter = vars.erase(oldIter, oldRemoveEnd);
            endRemoveRows();
        }
        // Insert new vars smaller than the next old var, or insert all if no more old vars
        auto newInsertEnd = newVars.end(); 
        if (oldIter != vars.end()) {
            newInsertEnd = std::find_if_not(newIter, newVars.end(), [=](const calc_var_t &var) { return varLess(var, oldIter->info); });
        }
        if (newIter != newInsertEnd) {
            int lastRow = row + (newInsertEnd - newIter - 1);
            beginInsertRows(QModelIndex(), row, lastRow);
            oldIter = vars.insert(oldIter, newIter, newInsertEnd);
            endInsertRows();
            // Handle header resize because it doesn't care about inserted rows
            emit dataChanged(index(row, 0), index(lastRow, VAR_NUM_COLS - 1), { Qt::SizeHintRole });
            oldIter += (newInsertEnd - newIter);
            newIter = newInsertEnd;
        } else {
            // No new vars were smaller, and the old var cannot be smaller because those were already removed, so they're equal
            assert(oldIter != vars.end());
            // Update the old variable with the new variable data
            uint8_t changed = oldIter->updateInfo(*newIter);
            // Inform the view of changed columns
            for (uint8_t col = 0; col < VAR_NUM_COLS; col++) {
                if (changed & (1 << col)) {
                    QModelIndex cell = index(row, col);
                    emit dataChanged(cell, cell);
                }
            }
            oldIter++;
            newIter++;
        }
    }
    // Remove any remaining old vars
    if (oldIter != vars.end()) {
        beginRemoveRows(QModelIndex(), oldIter - vars.begin(), vars.size() - 1);
        vars.erase(oldIter, vars.end());
        endRemoveRows();
    }
}

void VarTableModel::retranslate() {
    if (!vars.empty()) {
        for (VarData &var : vars) {
            // Only invalid previews are translated
            if (var.previewState == PreviewState::Invalid) {
                var.previewState = PreviewState::Outdated;
            }
        }
        emit dataChanged(index(0, VAR_LOCATION_COL), index(vars.size() - 1, VAR_NUM_COLS - 1));
    }
    emit headerDataChanged(Qt::Horizontal, 0, VAR_NUM_COLS - 1);
}

QVariant VarTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    VarData &var = vars[index.row()];
    if (index.column() == VAR_PREVIEW_COL && var.previewState == PreviewState::Outdated) {
        var.updatePreview();
    }
    const QByteArray key = variableKey(var.info);
    const VariableConfig config = variableConfigs.value(key);
    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case VAR_NAME_COL:
                    return variableName(var.info);
                case VAR_ALIAS_COL:
                    return config.alias;
                case VAR_LOCATION_COL:
                    return var.info.archived ? tr("Archive") : QStringLiteral("RAM");
                case VAR_TYPE_COL:
                {
                    // Do not translate - things rely on those names.
                    QString var_type_str = calc_var_type_names[var.info.type];
                    if (calc_var_is_asmprog(&var.info)) {
                        var_type_str += QStringLiteral(" (ASM)");
                    } else if (calc_var_is_python_appvar(&var.info)) {
                        var_type_str += QStringLiteral(" (Python)");
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
            if (index.column() == VAR_NAME_COL && config.watched) {
                QFont font;
                font.setBold(true);
                return font;
            }
            if (index.column() == VAR_PREVIEW_COL) {
                return var.previewState == PreviewState::Invalid ? varPreviewItalicFont : varPreviewCEFont;
            }
            return QVariant();
        case Qt::ForegroundRole:
            if (index.column() == VAR_PREVIEW_COL && var.previewState == PreviewState::Invalid) {
                return QBrush(Qt::gray);
            }
            return QVariant();
        case Qt::ToolTipRole:
            if (index.column() == VAR_NAME_COL || index.column() == VAR_ALIAS_COL) {
                QStringList details;
                if (!config.alias.isEmpty()) {
                    details.append(tr("Alias: %1").arg(config.alias));
                }
                if (config.watched) {
                    details.append(tr("Break on BASIC value change"));
                }
                return details.join(QLatin1Char('\n'));
            }
            return QVariant();
        case Qt::CheckStateRole:
            if (index.column() == VAR_NAME_COL) {
                return var.checked ? Qt::Checked : Qt::Unchecked;
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
        vars[index.row()].checked = qvariant_cast<Qt::CheckState>(value) == Qt::Checked;
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
            case VAR_ALIAS_COL:
                return tr("Alias");
            case VAR_LOCATION_COL:
                return tr("Location");
            case VAR_TYPE_COL:
                return tr("Type");
            case VAR_SIZE_COL:
                return tr("Size");
            case VAR_PREVIEW_COL:
                return tr("Value");
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

VarTableSortFilterModel::VarTableSortFilterModel(VarTableModel *parent) : QSortFilterProxyModel(parent), typeFilter(-1) {
    setSourceModel(parent);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortLocaleAware(true);
}

void VarTableSortFilterModel::setTypeFilter(int type) {
    if (type != typeFilter) {
        typeFilter = type;
        invalidateFilter();
        // Handle header resize because it doesn't care about inserted rows
        int rows = rowCount(), cols = columnCount();
        if (rows > 0 && cols > 0) {
            emit dataChanged(index(0, 0), index(rows - 1, cols - 1), { Qt::SizeHintRole });
        }
    }
}

bool VarTableSortFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    if (typeFilter < 0) {
        return true;
    }
    calc_var_t var = sourceModel()->index(source_row, VarTableModel::VAR_NAME_COL, source_parent).data(Qt::UserRole).value<calc_var_t>();
    return calc_var_normalized_type(var.type) == typeFilter;
}
