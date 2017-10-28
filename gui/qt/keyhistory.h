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
    void addEntry(const QString& entry);

private:
    void setFont(int size);
    void setTop(bool state);

    Ui::KeyHistory *ui;
};

#endif
