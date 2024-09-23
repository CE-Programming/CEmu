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

#ifndef PORTMONITORWIDGET_H
#define PORTMONITORWIDGET_H

#include "../dockedwidget.h"
class TableWidget;

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableWidgetItem;
QT_END_NAMESPACE

class PortMonitor
{
public:
    enum Mode
    {
        E = (1 << 0),
        R = (1 << 1),
        W = (1 << 2),
    };

    int mode;
    int port;
};

class PortMonitorWidget : public DockedWidget
{
    Q_OBJECT

public:
    explicit PortMonitorWidget(CoreWindow *coreWindow, const QList<PortMonitor> &portmonitors);

    void enableDebugWidgets(bool) override;
    void loadFromCore() override;
    void storeToCore() const override;

private slots:
    void addPortMonitor(const PortMonitor &portmonitor, bool edit);
    void setPortMonitorMode(int row, int mode);
    int getPortMonitorMode(int row);
    bool toggleMode(int row, int bit);
    void removeSelected();
    void itemPressed(QTableWidgetItem *item);
    void itemChanged(QTableWidgetItem *item);

private:
    enum Column
    {
        Enabled,
        Read,
        Write,
        Port,
        Data,
    };
    enum Role
    {
        Id = Qt::UserRole,
        Mode,
    };

    TableWidget *mTbl;

    QPushButton *mBtnRemoveSelected;

    QBrush mNormalBackground;

    static const QString mEnabledText;
    static const QString mDisabledText;
    static const QString mRdText;
    static const QString mWrText;
    static const QString mRdWrText;

    bool mInDebug;
};

#endif
