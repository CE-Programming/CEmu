#ifndef VARTABLEMODEL_H
#define VARTABLEMODEL_H

#include "../../core/vat.h"

#include <QtCore/QAbstractTableModel>
#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QFont>
#include <vector>

class VarTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum {
        VAR_NAME_COL,
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

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

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

    mutable std::vector<VarData> vars;
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
