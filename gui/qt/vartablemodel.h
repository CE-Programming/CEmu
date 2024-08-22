#ifndef VARTABLEMODEL_H
#define VARTABLEMODEL_H

#include "../../core/vat.h"

#include <QtCore/QAbstractTableModel>
#include <QtGui/QFont>

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
    struct VarData {
        explicit VarData(const calc_var_t &var);
        void updatePreview();

        calc_var_t info;
        QByteArray data;
        QString preview;
        bool previewValid;
        Qt::CheckState checked;
    };

    QVector<VarData> vars;
    QFont varPreviewItalicFont;
    QFont varPreviewCEFont;
};

#endif // VARTABLEMODEL_H
