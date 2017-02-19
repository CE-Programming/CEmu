#include <QtWidgets>
#include <QPushButton>

#include "keyhistory.h"
#include "ui_keyhistory.h"

KeyHistory::KeyHistory(QWidget *parent) : QWidget(parent), ui(new Ui::KeyHistory) {
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    connect(ui->buttonClear, &QPushButton::clicked, ui->historyView, &QPlainTextEdit::clear);
}

KeyHistory::~KeyHistory() {
    delete ui;
}

void KeyHistory::addEntry(QString entry) {
    ui->historyView->appendPlainText(entry);
}
