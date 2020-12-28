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

#include <QString>
#include <QSettings>

class Settings
{

public:
    explicit Settings(const QString &dirpath);
    ~Settings();

    static Settings* instance();

    static void setSettingsDirectory(const QString &dirpath);

    static bool boolOption(const QString &key);
    static void setBoolOption(const QString &key, bool boolOption);

    static QString textOption(const QString &key);
    static void setTextOption(const QString &key, const QString &textOption);

    static int intOption(const QString &key);
    static void setIntOption(const QString &key, int integerOption);

    static void setDefaults();
    static void setDefaultOption(const QString &key, QVariant value);
    static void saveSettings();

    static bool contains(const QString &key);

    // setting strings
    static const QString KeyMap;
    static const QString KeyMapCustom;
    static const QString KeypadColor;
    static const QString FirstRun;

    // layout strings
    static const QString LayoutFile;

    // key map strings
    static const QString KeyMapNatural;
    static const QString KeyMapCemu;
    static const QString KeyMapTilem;
    static const QString KeyMapWabbit;
    static const QString KeyMapJstified;

private:
    static Settings *sInstance;

    QSettings *mSettings;
    bool mHasPath;
};

#endif
