#ifndef KEYHISTORY_H
#define KEYHISTORY_H

#include <QWidget>

namespace Ui { class KeyHistory; }

class KeyHistory : public QWidget {
    Q_OBJECT

public:
    explicit KeyHistory(QWidget *p = Q_NULLPTR);
    ~KeyHistory();

public slots:
    void addEntry(QString entry);

private:
    Ui::KeyHistory *ui;
};

#endif
