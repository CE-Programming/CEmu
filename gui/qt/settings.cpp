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

#include "settings.h"

#include "keypad/keymap.h"
#include "keypad/keypadwidget.h"

#include <QtCore/QSettings>

const QString Settings::KeyMap            = QStringLiteral("keys/map");
const QString Settings::KeyMapCustom      = QStringLiteral("keys/custom");
const QString Settings::KeyHistoryFont    = QStringLiteral("keyhistory/font");
const QString Settings::KeypadColor       = QStringLiteral("keypad/color");
const QString Settings::LayoutFile        = QStringLiteral("layout/file");
const QString Settings::RomFile           = QStringLiteral("files/rom");
const QString Settings::ConsoleAutoScroll = QStringLiteral("console/autoscroll");
const QString Settings::AutoUpdate        = QStringLiteral("general/autoupdate");
const QString Settings::PortableMode      = QStringLiteral("general/portable");
const QString Settings::EmuThrottle       = QStringLiteral("emu/throttle");
const QString Settings::EmuSpeed          = QStringLiteral("emu/speed");
const QString Settings::EmuPreI           = QStringLiteral("emu/prei");
const QString Settings::EmuLcdSpi         = QStringLiteral("emu/lcdspi");
const QString Settings::EmuFrameSkip      = QStringLiteral("display/frameskip_enabled");
const QString Settings::EmuFrameSkipRate  = QStringLiteral("display/frameskip");
const QString Settings::DevSoftCmds       = QStringLiteral("developer/softcmds");
const QString Settings::DevTIOS           = QStringLiteral("developer/tios");
const QString Settings::DevOpenDebug      = QStringLiteral("developer/resetnmi");
const QString Settings::SettingsPath      = QStringLiteral("preferences/file");
const QString Settings::StatesPath        = QStringLiteral("states/path");
const QString Settings::Language          = QStringLiteral("preferences/language");

Settings *Settings::sInstance = nullptr;

Settings::Settings(const QString &dirpath)
{
    Q_ASSERT(sInstance == nullptr);
    sInstance = this;

    QSettings::setDefaultFormat(QSettings::IniFormat);
    sInstance->mSettings = new QSettings(dirpath + "/config/preferences.conf", QSettings::IniFormat);

    setTextOption(Settings::SettingsPath, dirpath + "/config");
    setTextOption(Settings::StatesPath, dirpath + "/states");
    setTextOption(Settings::LayoutFile, dirpath + "/config/layout.json");

    setDefaults(false);
}

Settings::~Settings()
{
    Q_ASSERT(sInstance != nullptr);
    delete sInstance->mSettings;
    sInstance = nullptr;
}

void Settings::setDefaults(bool force)
{
    setDefaultOption(force, Settings::KeyMap, Keymap::CEmu);
    setDefaultOption(force, Settings::KeypadColor, KeypadWidget::Color::Denim);
    setDefaultOption(force, Settings::KeyMapCustom, QStringLiteral("none"));
    setDefaultOption(force, Settings::KeyHistoryFont, 9);
    setDefaultOption(force, Settings::ConsoleAutoScroll, true);
    setDefaultOption(force, Settings::AutoUpdate, true);
    setDefaultOption(force, Settings::PortableMode, false);
    setDefaultOption(force, Settings::EmuThrottle, true);
    setDefaultOption(force, Settings::EmuSpeed, 100);
    setDefaultOption(force, Settings::EmuPreI, false);
    setDefaultOption(force, Settings::EmuLcdSpi, true);
    setDefaultOption(force, Settings::EmuFrameSkip, false);
    setDefaultOption(force, Settings::EmuFrameSkipRate, 0);
    setDefaultOption(force, Settings::DevSoftCmds, true);
    setDefaultOption(force, Settings::DevTIOS, true);
    setDefaultOption(force, Settings::DevOpenDebug, false);
    setDefaultOption(force, Settings::Language, Lang::English);

    setDefaultOption(false, Settings::RomFile, QStringLiteral("none"));

    saveSettings();
}

void Settings::saveSettings()
{
    sInstance->mSettings->sync();
}

bool Settings::boolOption(const QString &key)
{
    return sInstance->mSettings->value(key).toBool();
}

void Settings::setBoolOption(const QString &key, bool boolOption)
{
    sInstance->mSettings->setValue(key, boolOption);
    saveSettings();
}

QString Settings::textOption(const QString &key)
{
    return sInstance->mSettings->value(key).toString();
}

void Settings::setTextOption(const QString &key, const QString &textOption)
{
    sInstance->mSettings->setValue(key, textOption);
    saveSettings();
}

int Settings::intOption(const QString &key)
{
    return sInstance->mSettings->value(key).toInt();
}

void Settings::setIntOption(const QString &key, int integerOption)
{
    sInstance->mSettings->setValue(key, integerOption);
    saveSettings();
}

bool Settings::contains(const QString &key)
{
    return sInstance->mSettings->contains(key);
}

void Settings::setDefaultOption(bool force, const QString &key, const QVariant &value)
{
    if (force || !contains(key))
    {
        sInstance->mSettings->setValue(key, value);
    }
}

