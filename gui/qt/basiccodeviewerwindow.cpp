#include "basiccodeviewerwindow.h"
#include "ui_basiccodeviewerwindow.h"

BasicCodeViewerWindow::BasicCodeViewerWindow(QWidget *p) : QDialog(p), ui(new Ui::BasicCodeViewerWindow) {
    ui->setupUi(this);
    setWindowTitle(tr("Variable viewer"));
    ui->plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void BasicCodeViewerWindow::setVariableName(const QString &name) {
    variableName = name;
    setWindowTitle(tr("Variable viewer") + " | " + variableName);
}

void BasicCodeViewerWindow::on_pushButton_clicked() {
    showingFormatted = !showingFormatted;
    showCode();
}

void BasicCodeViewerWindow::showCode() {
    ui->plainTextEdit->document()->setPlainText(showingFormatted ? formattedCode : originalCode);
}

BasicCodeViewerWindow::~BasicCodeViewerWindow() {
    delete ui;
}
