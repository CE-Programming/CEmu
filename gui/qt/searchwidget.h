#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QtWidgets/QDialog>

namespace Ui { class searchwidget; }

class SearchWidget : public QDialog {
    Q_OBJECT

public:
    explicit SearchWidget(const QString&, int, QWidget *p = Q_NULLPTR);
    ~SearchWidget();
    int getMode() const;
    int getType() const;
    QString getSearchString() const;

    enum {
        Cancel,
        Next,
        NextNot,
        Prev,
        PrevNot
    };

    enum {
        Hex,
        Ascii
    };

public slots:
    void findNext();
    void findNextNot();
    void findPrev();
    void findPrevNot();
    void changeInputHEX();
    void changeInputASCII();

private:
    int m_searchMode = Hex;
    int m_searchType = Cancel;
    Ui::searchwidget *ui;
};

#endif // SEARCHWIDGET_H
