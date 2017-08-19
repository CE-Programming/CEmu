#include "basiccodeviewerwindow.h"
#include "ui_basiccodeviewerwindow.h"

BasicCodeViewerWindow::BasicCodeViewerWindow(QWidget *p) : QDialog(p), ui(new Ui::BasicCodeViewerWindow) {
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, &BasicCodeViewerWindow::toggleFormat);

    // Add special jacobly font
    QFont font = QFont(QStringLiteral("TICELarge"), 9);
    ui->plainTextEdit->setFont(font);
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
