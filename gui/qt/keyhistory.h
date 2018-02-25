#ifndef KEYHISTORY_H
#define KEYHISTORY_H

#include <QWidget>

namespace Ui { class KeyHistory; }

class KeyHistory : public QWidget {
    Q_OBJECT

public:
    explicit KeyHistory(QWidget *p = Q_NULLPTR);
    ~KeyHistory();

signals:
    void closed();

public slots:
    void addEntry(const QString& entry);

protected slots:
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    void setFont(int size);
    void setTop(bool state);

    Ui::KeyHistory *ui;
};

#endif
