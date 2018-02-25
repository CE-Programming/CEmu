#include <QtWidgets>
#include <QPushButton>

#include "keyhistory.h"
#include "ui_keyhistory.h"

KeyHistory::KeyHistory(QWidget *parent) : QWidget(parent), ui(new Ui::KeyHistory) {
    ui->setupUi(this);
    connect(ui->buttonClear, &QPushButton::clicked, ui->historyView, &QPlainTextEdit::clear);
    connect(ui->textSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KeyHistory::setFont);
    connect(ui->checkOnTop, &QCheckBox::clicked, this, &KeyHistory::setTop);
    setTop(true);
    setFont(9);
}

KeyHistory::~KeyHistory() {
    delete ui;
}

void KeyHistory::addEntry(const QString& entry) {
    ui->historyView->appendPlainText(entry);
}

void KeyHistory::setTop(bool state) {
    if (!state) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    ui->checkOnTop->setChecked(state);
    show();
}

void KeyHistory::setFont(int size) {
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->textSize->setValue(size);
    monospace.setPointSize(size);
    ui->historyView->setFont(monospace);
}

void KeyHistory::closeEvent(QCloseEvent *event) {
    emit closed();
    event->accept();
    QWidget::closeEvent(event);
}
