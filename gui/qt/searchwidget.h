#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QtWidgets/QDialog>

namespace Ui { class searchwidget; }

enum {
    SEARCH_CANCEL=0,
    SEARCH_NEXT,
    SEARCH_NEXT_NOT,
    SEARCH_PREV,
    SEARCH_PREV_NOT,
    SEARCH_MAX
};

enum {
    SEARCH_MODE_HEX=0,
    SEARCH_MODE_ASCII
};

class SearchWidget : public QDialog {
    Q_OBJECT

public:
    explicit SearchWidget(const QString&, int, QWidget *p = Q_NULLPTR);
    ~SearchWidget();

    int getMode();
    int getType();
    QString getSearchString();

public slots:
    void findNext();
    void findNextNot();
    void findPrev();
    void findPrevNot();
    void changeInputHEX();
    void changeInputASCII();

private:
    int m_searchMode = SEARCH_MODE_HEX;
    int m_searchType = SEARCH_CANCEL;
    Ui::searchwidget *ui;
};

#endif // SEARCHWIDGET_H
