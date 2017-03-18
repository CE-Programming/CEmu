#include "searchwidget.h"
#include "ui_searchwidget.h"

#include <QtCore/QDebug>

SearchWidget::SearchWidget(const QString &line, int type, QWidget *p) : QDialog(p), ui(new Ui::searchwidget) {
    searchType = type;
    bool mode = (type == SEARCH_MODE_HEX);
    ui->setupUi(this);

    ui->searchEdit->setText(line);
    ui->searchEdit->selectAll();

    ui->radioHEX->setChecked(mode);
    ui->radioASCII->setChecked(!mode);

    connect(ui->buttonCancel, &QPushButton::clicked, this, &SearchWidget::close);
    connect(ui->buttonFind, &QPushButton::clicked, this, &SearchWidget::findNext);
    connect(ui->radioASCII, &QRadioButton::clicked, this, &SearchWidget::changeInputASCII);
    connect(ui->radioHEX, &QRadioButton::clicked, this, &SearchWidget::changeInputHEX);
    connect(ui->buttonFindNot, &QPushButton::clicked, this, &SearchWidget::findNextNot);
    connect(ui->buttonPrev, &QPushButton::clicked, this, &SearchWidget::findPrev);
    connect(ui->buttonPrevNot, &QPushButton::clicked, this, &SearchWidget::findPrevNot);
}

SearchWidget::~SearchWidget() {
    delete ui;
}

int SearchWidget::getType() {
    return searchType;
}

int SearchWidget::getMode() {
    return searchMode;
}

QString SearchWidget::getSearchString() {
    return ui->searchEdit->text();
}

void SearchWidget::findNext() {
    searchMode = SEARCH_NEXT;
    done(searchMode);
}

void SearchWidget::findNextNot() {
    searchMode = SEARCH_NEXT_NOT;
    done(searchMode);
}

void SearchWidget::findPrev() {
    searchMode = SEARCH_PREV;
    done(searchMode);
}

void SearchWidget::findPrevNot() {
    searchMode = SEARCH_PREV_NOT;
    done(searchMode);
}

void SearchWidget::changeInputASCII() {
    ui->radioASCII->setChecked(true);
    ui->radioHEX->setChecked(false);
    searchType = SEARCH_MODE_ASCII;
}

void SearchWidget::changeInputHEX() {
    ui->radioHEX->setChecked(true);
    ui->radioASCII->setChecked(false);
    searchType = SEARCH_MODE_HEX;
}

