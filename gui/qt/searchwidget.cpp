#include "searchwidget.h"
#include "ui_searchwidget.h"

SearchWidget::SearchWidget(const QString &line, int type, QWidget *parent) : QDialog{parent}, ui(new Ui::searchwidget) {
    m_searchType = type;
    bool mode = (type == Hex);
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
    return m_searchType;
}

int SearchWidget::getMode() {
    return m_searchMode;
}

QString SearchWidget::getSearchString() {
    return ui->searchEdit->text();
}

void SearchWidget::findNext() {
    m_searchMode = Next;
    done(m_searchMode);
}

void SearchWidget::findNextNot() {
    m_searchMode = NextNot;
    done(m_searchMode);
}

void SearchWidget::findPrev() {
    m_searchMode = Prev;
    done(m_searchMode);
}

void SearchWidget::findPrevNot() {
    m_searchMode = PrevNot;
    done(m_searchMode);
}

void SearchWidget::changeInputASCII() {
    ui->radioASCII->setChecked(true);
    ui->radioHEX->setChecked(false);
    m_searchType = Ascii;
}

void SearchWidget::changeInputHEX() {
    ui->radioHEX->setChecked(true);
    ui->radioASCII->setChecked(false);
    m_searchType = Hex;
}

