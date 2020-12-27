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

#ifndef KEYHISTORYWIDGET_H
#define KEYHISTORYWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSpacerItem>
#include <QLabel>
#include <QSpinBox>

class KeyHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyHistoryWidget(QWidget *parent = nullptr);
    ~KeyHistoryWidget();

public slots:
    void add(const QString &entry);
    int getFontSize();

signals:
    void fontSizeChanged();

private:
    void setFontSize(int size);

    QLabel *mLblSize;
    QSpinBox *mFontSize;
    QSpacerItem *mSpacer;
    QPlainTextEdit *mText;
    QPushButton *mBtnClear;
};

#endif
