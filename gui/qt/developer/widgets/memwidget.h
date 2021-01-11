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

#ifndef MEMWIDGET_H
#define MEMWIDGET_H

class DockedWidget;
class HexWidget;

namespace KDDockWidgets
{
class DockWidget;
}

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

class MemWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Area
    {
        Mem,
        Flash,
        Ram,
        Port,
    };

    explicit MemWidget(DockedWidget *dockedWidget, Area area = Area::Mem);
    DockedWidget *dockedWidget() const;

private:
    void showSearchDialog();
    void selectCharset();

    DockedWidget *mDockedWidget;
    QPushButton *mBtnCharset;
    HexWidget *mView;
    QString mSearch;
    QLineEdit *mEdtAddr;
    QString mCharsetNoneText;
    QString mCharsetTiAsciiText;
    QString mCharsetAsciiText;
    bool mSearchHex;
};

#endif
