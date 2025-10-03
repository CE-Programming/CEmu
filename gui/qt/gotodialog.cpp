#include "gotodialog.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QSizePolicy>

GotoDialog::GotoDialog(const QString &seed,
                       const std::vector<QString> &history,
                       QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Goto"));
    auto *layout = new QVBoxLayout(this); // NOLINT: Qt parent-ownership handles deletion

    auto *label = new QLabel(tr("Input Address (Or Equate):"), this); // NOLINT: Qt parent-ownership handles deletion
    layout->addWidget(label);

    m_combo = new QComboBox(this); // NOLINT: Qt parent-ownership handles deletion
    m_combo->setEditable(true);

    for (const QString &h : history) {
        m_combo->addItem(h);
    }
    if (!seed.isEmpty()) {
        m_combo->setEditText(seed.toUpper());
    }
    layout->addWidget(m_combo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this); // NOLINT: Qt parent-ownership handles deletion
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    layout->setSizeConstraint(QLayout::SetFixedSize);
    setMinimumWidth(360);

    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    m_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_combo->setFixedHeight(m_combo->sizeHint().height());
    adjustSize();
}

QString GotoDialog::text() const {
    return m_combo ? m_combo->currentText() : QString();
}
