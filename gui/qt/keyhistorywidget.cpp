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

    m_view->setReadOnly(true);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    hlayout->addWidget(m_btnClear);
    hlayout->addSpacerItem(m_spacer);
    hlayout->addWidget(m_label);
    hlayout->addWidget(m_size);

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
    m_view->moveCursor(QTextCursor::End);
    m_view->insertPlainText(entry);
    m_view->moveCursor(QTextCursor::End);
}

void KeyHistoryWidget::setFontSize(int size) {
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_size->setValue(size);
    monospace.setPointSize(size);
    m_view->setFont(monospace);
    emit fontSizeChanged();
}

int KeyHistoryWidget::getFontSize() {
    return m_size->value();
}
