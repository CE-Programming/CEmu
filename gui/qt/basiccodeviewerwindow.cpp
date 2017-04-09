#include "basiccodeviewerwindow.h"
#include "ui_basiccodeviewerwindow.h"

BasicCodeViewerWindow::BasicCodeViewerWindow(QWidget *p) : QDialog(p), ui(new Ui::BasicCodeViewerWindow) {
    ui->setupUi(this);
    ui->plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    connect(ui->pushButton, &QPushButton::clicked, this, &BasicCodeViewerWindow::toggleFormat);
}

void BasicCodeViewerWindow::setVariableName(const QString &name) {
    variableName = name;
    setWindowTitle(tr("Variable viewer") + QStringLiteral(" | ") + variableName);
}

void BasicCodeViewerWindow::toggleFormat() {
    showingFormatted ^= true;
    showCode();
}

void BasicCodeViewerWindow::showCode() {
    ui->plainTextEdit->document()->setPlainText(showingFormatted ? formattedCode : originalCode);
}

BasicCodeViewerWindow::~BasicCodeViewerWindow() {
    delete ui;
}
