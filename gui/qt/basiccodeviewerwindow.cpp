#include "basiccodeviewerwindow.h"
#include "ui_basiccodeviewerwindow.h"

BasicCodeViewerWindow::BasicCodeViewerWindow(QWidget *parent) : QDialog{parent}, ui(new Ui::BasicCodeViewerWindow) {
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, &BasicCodeViewerWindow::toggleFormat);

    // Add special jacobly font
    QFont font = QFont(QStringLiteral("TICELarge"), 9);
    ui->plainTextEdit->setFont(font);
}

void BasicCodeViewerWindow::setVariableName(const QString &name) {
    m_variableName = name;
    setWindowTitle(tr("Variable viewer") + QStringLiteral(" | ") + m_variableName);
}

void BasicCodeViewerWindow::toggleFormat() {
    m_showingFormatted ^= true;
    showCode();
}

void BasicCodeViewerWindow::showCode() {
    ui->plainTextEdit->document()->setPlainText(m_showingFormatted ? m_formattedCode : m_originalCode);
}

BasicCodeViewerWindow::~BasicCodeViewerWindow() {
    delete ui;
}
