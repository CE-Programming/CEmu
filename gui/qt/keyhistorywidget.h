#ifndef KEYHISTORYWIDGET_H
#define KEYHISTORYWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>

class KeyHistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeyHistoryWidget(QWidget *parent = Q_NULLPTR, int size = 9);
    ~KeyHistoryWidget();

public slots:
    void add(const QString &entry);
    int getFontSize();

signals:
    void fontSizeChanged();

private:
    void setFontSize(int size);

    QLabel *m_label;
    QSpinBox *m_size;
    QSpacerItem *m_spacer;
    QPlainTextEdit *m_view;
    QPushButton *m_btnClear;
    QCheckBox *m_chkBoxVertical;
};

#endif
