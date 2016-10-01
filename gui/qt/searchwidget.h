#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QtWidgets/QDialog>

namespace Ui { class searchwidget; }

class SearchWidget : public QDialog {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *p = Q_NULLPTR);
    ~SearchWidget();

    void setSearchString(QString);
    void setInputMode(bool);
    bool getInputMode();
    bool getStatus();
    QString getSearchString();

public slots:
    void changeInputASCII();
    void changeInputHEX();
    void find();

private:
    bool status = false;
    Ui::searchwidget *ui;
};

#endif // SEARCHWIDGET_H
