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

#ifndef MEMWIDGET_H
#define MEMWIDGET_H

class DockedWidget;
class CoreWrapper;
class HexWidget;

namespace KDDockWidgets
{
class DockWidget;
}

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE
class QGroupBox;
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

    explicit MemWidget(DockedWidget *dockedWidget);
    DockedWidget *parent() const;
    CoreWrapper &core() const;
    Area area() const;
    void setArea(Area area);

private slots:
    void showSearchGroup();
    void selectCharset();
    void selectArea();
    void selectSearchType();

private:
    DockedWidget *mDockedWidget;
    QPushButton *mBtnCharset;
    QPushButton *mBtnArea;
    QPushButton *mBtnSearch;
    QPushButton *mBtnSearchType;
    HexWidget *mView;
    QString mSearch;
    QLineEdit *mEdtAddr;
    QString mCharsetNoneText;
    QString mCharsetTiAsciiText;
    QString mCharsetAsciiText;
    QString mAreaRamText;
    QString mAreaFlashText;
    QString mAreaMemoryText;
    QString mAreaPortsText;
    QString mSearchHexText;
    QString mSearchAsciiText;
    QGroupBox *mGrpSearch;
    Area mArea;
    bool mSearchHex;
};

#endif
