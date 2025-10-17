#include "gotodialog.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QLineEdit>
#include <QtCore/Qt>

GotoDialog::GotoDialog(const QString &seed,
                       const std::vector<QString> &history,
                       const QStringList &completions,
                       QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("Goto"));
    auto *layout = new QVBoxLayout(this); // NOLINT: Qt parent-ownership handles deletion

    auto *label = new QLabel(tr("Input Address (Or Equate):"), this); // NOLINT: Qt parent-ownership handles deletion
    layout->addWidget(label);

    m_combo = new QComboBox(this); // NOLINT: Qt parent-ownership handles deletion
    m_combo->setEditable(true);
    m_combo->setInsertPolicy(QComboBox::NoInsert);

    if (!completions.isEmpty()) {
        auto *completer = new QCompleter(completions, m_combo); // NOLINT: Qt parent-ownership handles deletion
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        if (auto *lineEdit = m_combo->lineEdit()) {
            lineEdit->setCompleter(completer);
        } else {
            m_combo->setCompleter(completer);
        }
    }

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
