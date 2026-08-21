#ifndef VARTABLEMODEL_H
#define VARTABLEMODEL_H

#include "../../core/vat.h"

#include <QtCore/QAbstractTableModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QStringList>
#include <QtGui/QFont>
#include <vector>

class VarTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum {
        VAR_NAME_COL,
        VAR_ALIAS_COL,
        VAR_LOCATION_COL,
        VAR_TYPE_COL,
        VAR_SIZE_COL,
        VAR_PREVIEW_COL,
        VAR_NUM_COLS
    };

    explicit VarTableModel(QObject *parent = Q_NULLPTR);

    void clear();
    void refresh();
    void retranslate();
    QString alias(const QModelIndex &index) const;
    void setAlias(const QModelIndex &index, const QString &alias);
    bool isWatched(const QModelIndex &index) const;
    void setWatched(const QModelIndex &index, bool watched);
    bool hasWatchedVariables() const;
    QStringList checkWatchedVariables();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

signals:
    void watchedVariablesChanged();

private:
    enum class PreviewState : uint8_t {
        Outdated,
        Invalid,
        Valid
    };

    struct VarData {
        VarData(const calc_var_t &var);
        ~VarData();
        VarData(VarData &&other) noexcept;
        VarData &operator=(VarData &&other) noexcept;

        uint8_t updateInfo(const calc_var_t &var);
        void updatePreview();

        calc_var_t info;
        QString preview;
        PreviewState previewState;
        bool checked;
    };

    struct VariableConfig {
        QString alias;
        QString name;
        QByteArray watchedData;
        bool watched = false;
        bool watchInitialized = false;
        bool present = false;
    };

    static QByteArray variableKey(const calc_var_t &var);
    static QString variableName(const calc_var_t &var);
    QString configuredName(const QByteArray &key, const calc_var_t *var = nullptr) const;

    mutable std::vector<VarData> vars;
    QHash<QByteArray, VariableConfig> variableConfigs;
    QFont varPreviewItalicFont;
    QFont varPreviewCEFont;
};

class VarTableSortFilterModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit VarTableSortFilterModel(VarTableModel *parent);

    void setTypeFilter(int type);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    int typeFilter;
};

#endif // VARTABLEMODEL_H
