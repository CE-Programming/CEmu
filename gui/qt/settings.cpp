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

#include <QDebug>
#include <QStandardPaths>

const QString Settings::KeyMap            = QStringLiteral("keys/map");
const QString Settings::KeyMapCustom      = QStringLiteral("keys/custom");
const QString Settings::KeypadColor       = QStringLiteral("keypad/color");
const QString Settings::LayoutFile        = QStringLiteral("layout/file");

Settings *Settings::sInstance = nullptr;

Settings::Settings(const QString &dirpath)
{
    Q_ASSERT(sInstance == nullptr);
    sInstance = this;

    QSettings::setDefaultFormat(QSettings::IniFormat);
    sInstance->mSettings = new QSettings(dirpath + "/config/preferences.conf", QSettings::IniFormat);

    qDebug() << "path: " << sInstance->mSettings->fileName();
    setTextOption(Settings::LayoutFile, dirpath + "/config/layout.json");
}

Settings::~Settings()
{
    Q_ASSERT(sInstance != nullptr);
    delete sInstance->mSettings;
    sInstance = nullptr;
}

void Settings::setDefaults()
{
    setDefaultOption(Settings::KeyMap, Keymap::CEmu);
    setDefaultOption(Settings::KeypadColor, KeypadWidget::Color::Denim);
    setDefaultOption(Settings::KeyMapCustom, QStringLiteral("none"));
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

void Settings::setDefaultOption(const QString &key, QVariant value)
{
    if (!contains(key))
    {
        sInstance->mSettings->setValue(key, value);
    }
}

