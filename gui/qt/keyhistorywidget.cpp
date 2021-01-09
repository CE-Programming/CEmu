/*
 * Copyright (c) 2015-2020 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "keyhistorywidget.h"
#include "settings.h"

#include <kddockwidgets/DockWidget.h>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

KeyHistoryWidget::KeyHistoryWidget(CoreWindow *coreWindow)
    : DockedWidget{new KDDockWidgets::DockWidget{QStringLiteral("Key History")},
                   QIcon(QStringLiteral(":/assets/icons/kindle.svg")),
                   coreWindow}
{
    QPushButton *btnClear = new QPushButton(QIcon(QStringLiteral(":/assets/icons/empty_trash.svg")), tr("Clear History"), this);
    QLabel *lblSize = new QLabel(tr("Font size") + ':', this);

    mText = new QPlainTextEdit;
    mFontSize = new QSpinBox;

    mText->setReadOnly(true);
    mText->setMaximumBlockCount(1000);
    mText->setMinimumSize(10, 100);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(lblSize);
    hLayout->addWidget(mFontSize);
    hLayout->addStretch();
    hLayout->addWidget(btnClear);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(mText);
    vLayout->addLayout(hLayout);
    setLayout(vLayout);

    setFontSize(Settings::intOption(Settings::KeyHistoryFont));

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(btnClear, &QPushButton::clicked, mText, &QPlainTextEdit::clear);
    connect(mFontSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &KeyHistoryWidget::setFontSize);
}

KeyHistoryWidget::~KeyHistoryWidget()
{
}

void KeyHistoryWidget::add(const QString &entry)
{
    mText->moveCursor(QTextCursor::End);
    mText->insertPlainText(entry);
    mText->moveCursor(QTextCursor::End);
}

void KeyHistoryWidget::setFontSize(int size)
{
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monospace.setStyleHint(QFont::Monospace);
    mFontSize->setValue(size);
    monospace.setPointSize(size);
    mText->setFont(monospace);

    Settings::setIntOption(Settings::KeyHistoryFont, size);
}

int KeyHistoryWidget::getFontSize()
{
    return mFontSize->value();
}
