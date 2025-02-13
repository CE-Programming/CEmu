/*
 * Copyright (c) 2015-2021 CE Programming.
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

#ifndef HIGHLIGHTWIDGET
#define HIGHLIGHTWIDGET

#include <QtWidgets/QLineEdit>
QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

class HighlightEditWidget : public QLineEdit
{
    Q_OBJECT

public:
    explicit HighlightEditWidget(const QString &mask = QString(), QWidget *parent = nullptr);

public slots:
    void setString(const QString &str);
    void setInt(qulonglong value, int length = 6);
    qulonglong getInt();
    void clearHighlight();

private:
    QPalette mPaletteHighlight, mPaletteNoHighlight;
};

#endif
