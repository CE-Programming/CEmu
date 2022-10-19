#include "keyhistorywidget.h"

#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>

KeyHistoryWidget::KeyHistoryWidget(QWidget *parent, int size) : QWidget{parent} {
    QHBoxLayout *hlayout = new QHBoxLayout();

    m_btnClear = new QPushButton(tr("Clear History"));
    m_label = new QLabel(tr("Size"));
    m_view = new QPlainTextEdit();
    m_size = new QSpinBox();
    m_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_chkBoxVertical = new QCheckBox(tr("Print Vertically"));

    m_view->setReadOnly(true);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    hlayout->addWidget(m_btnClear);
    hlayout->addSpacerItem(m_spacer);
    hlayout->addWidget(m_label);
    hlayout->addWidget(m_size);
    hlayout->addWidget(m_chkBoxVertical);

    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addWidget(m_view);
    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    connect(m_btnClear, &QPushButton::clicked, m_view, &QPlainTextEdit::clear);
    connect(m_size, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KeyHistoryWidget::setFontSize);

    setFontSize(size);
}

KeyHistoryWidget::~KeyHistoryWidget() = default;

void KeyHistoryWidget::add(const QString &entry) {
    QString key = getText(entry);
    m_view->moveCursor(QTextCursor::End);
    m_view->insertPlainText(key);
    m_view->moveCursor(QTextCursor::End);
}

QString KeyHistoryWidget::getText(const QString &entry){
    QString output = "";
    QString verticalOption = "";
    if (m_chkBoxVertical->checkState() == Qt::CheckState::Checked) {
        verticalOption = "\n";
    }
    output = verticalOption + entry;
    return output;
}

void KeyHistoryWidget::setFontSize(int size) {
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monospace.setStyleHint(QFont::Monospace);
    m_size->setValue(size);
    monospace.setPointSize(size);
    m_view->setFont(monospace);
    emit fontSizeChanged();
}

int KeyHistoryWidget::getFontSize() {
    return m_size->value();
}
