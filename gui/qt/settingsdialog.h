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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtCore/QString>
#include <QtWidgets/QDialog>
QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QButtonGroup;
class QComboBox;
class QRadioButton;
class QTabWidget;
class QCheckBox;
class QSpinBox;
class QWidget;
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    int checkForReset() { return mReset; }

signals:
    void changedKeypadColor(int id);

private:
    QDialogButtonBox *mBtnBox;
    QTabWidget *mTabWidget;

    int mReset;
};

class SettingsResetTab : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsResetTab(QWidget *parent = nullptr);

signals:
    void reset(int mode);
};

class SettingsDeveloperTab : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsDeveloperTab(QWidget *parent = nullptr);

public slots:
    void saveSettings();

private:
    QCheckBox *mChkSoftCmds;
    QCheckBox *mChkTiOs;
    QCheckBox *mChkResetNmi;
};

class SettingsEmulationTab : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsEmulationTab(QWidget *parent = nullptr);

public slots:
    void saveSettings();

signals:
    void changedKeypadColor(int id);

private:
    QSpinBox *mSpnSpeed;
    QSpinBox *mSpnFrameSkip;
    QCheckBox *mChkThrottle;
    QCheckBox *mChkFrameSkip;
    QCheckBox *mChkLcdSpi;
    QCheckBox *mChkPreI;
    int mKeypadColor;
};

class SettingsGeneralTab : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsGeneralTab(QWidget *parent = nullptr);

public slots:
    void saveSettings();

signals:
    void changeLanguage();
    void changePortable();

private:
    QCheckBox *mChkAutoUpdate;
    QCheckBox *mChkPortable;
    QButtonGroup *mKeybind;
    QComboBox *mCmbLang;
    int mLang;
    bool mPortable;
};

class KeyColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyColorDialog(int color, QWidget *parent = nullptr);
    int getColor();

signals:
    void changedColor(int id);

private:
    QButtonGroup *mColors;
};

#endif
