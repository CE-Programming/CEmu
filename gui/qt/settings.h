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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore/QString>
QT_BEGIN_NAMESPACE
class QSettings;
class QVariant;
QT_END_NAMESPACE

class Settings
{
public:
    explicit Settings(const QString &dirpath);
    ~Settings();

    enum Lang
    {
        English,
        French,
        Spanish,
        NumberOfLangs
    };

    enum Reset
    {
        None,
        Langauge,
        Defaults,
        Gui,
        All
    };

    static Settings* instance();

    static bool boolOption(const QString &key);
    static void setBoolOption(const QString &key, bool boolOption);

    static QString textOption(const QString &key);
    static void setTextOption(const QString &key, const QString &textOption);

    static int intOption(const QString &key);
    static void setIntOption(const QString &key, int integerOption);

    static void setDefaults(bool force);
    static void saveSettings();

    static bool contains(const QString &key);

    static const QString ClearWindow;
    static const QString KeyMap;
    static const QString KeyMapCustom;
    static const QString KeyHistoryFont;
    static const QString KeypadColor;
    static const QString FirstRun;
    static const QString LayoutFile;
    static const QString KeyMapNatural;
    static const QString KeyMapCemu;
    static const QString KeyMapTilem;
    static const QString KeyMapWabbit;
    static const QString KeyMapJstified;
    static const QString ConsoleAutoScroll;
    static const QString RomFile;
    static const QString AutoUpdate;
    static const QString PortableMode;
    static const QString EmuThrottle;
    static const QString EmuSpeed;
    static const QString EmuPreI;
    static const QString EmuLcdSpi;
    static const QString EmuFrameSkip;
    static const QString EmuFrameSkipRate;
    static const QString DevSoftCmds;
    static const QString DevTIOS;
    static const QString DevOpenDebug;
    static const QString SettingsPath;
    static const QString Language;

private:
    static void setDefaultOption(bool force, const QString &key, const QVariant &value);

    static Settings *sInstance;

    QSettings *mSettings;
    bool mHasPath;
};

#endif
