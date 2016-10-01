#include "searchwidget.h"
#include "ui_searchwidget.h"

#include <QtCore/QDebug>

SearchWidget::SearchWidget(QWidget *p) : QDialog(p), ui(new Ui::searchwidget) {
    ui->setupUi(this);

    connect(ui->buttonCancel, &QPushButton::clicked, this, &SearchWidget::close);
    connect(ui->buttonFind, &QPushButton::clicked, this, &SearchWidget::find);
    connect(ui->radioASCII, &QRadioButton::clicked, this, &SearchWidget::changeInputASCII);
    connect(ui->radioHEX, &QRadioButton::clicked, this, &SearchWidget::changeInputHEX);
}

SearchWidget::~SearchWidget() {
    delete ui;
}

void SearchWidget::setSearchString(QString line) {
    ui->searchEdit->setText(line);
    ui->searchEdit->selectAll();
}

void SearchWidget::setInputMode(bool type) {
    ui->radioHEX->setChecked(type);
    ui->radioASCII->setChecked(!type);
}

bool SearchWidget::getInputMode() {
    return ui->radioHEX->isChecked();
}

QString SearchWidget::getSearchString() {
    QString text;
    text = ui->searchEdit->text();
    return text;
}

bool SearchWidget::getStatus() {
    return status;
}

void SearchWidget::find() {
    status = true;
    close();
}

void SearchWidget::changeInputASCII() {
    ui->radioASCII->setChecked(true);
    ui->radioHEX->setChecked(false);
}

void SearchWidget::changeInputHEX() {
    ui->radioHEX->setChecked(true);
    ui->radioASCII->setChecked(false);
}

