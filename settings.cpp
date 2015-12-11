#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include "settings.h"

// Global static pointer used to ensure a single instance of the class.
CEmuSettings* CEmuSettings::m_pInstance = NULL;

CEmuSettings::CEmuSettings()
{
    // traverse to AppData and try to open things there
    QString settings_path = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(settings_path);
    if (!dir.exists()) {
        dir.mkpath(settings_path);
    }
    if (!dir.exists("CEmu")) {   // create the directory if it doesn't exist
        dir.mkdir("CEmu");
    }
    dir.cd("CEmu");             // move to the CEmu directory
    settings_path = dir.absoluteFilePath("CEmu.ini");

    CEmu_settings = new QSettings(settings_path,QSettings::IniFormat);
}

CEmuSettings* CEmuSettings::Instance()
{
    if (!m_pInstance) {  // Only allow one instance of class to be generated.
        m_pInstance = new CEmuSettings;
    }

    return m_pInstance;
}

CEmuSettings::~CEmuSettings()
{
    delete CEmu_settings;
    CEmu_settings = NULL;
}

void CEmuSettings::setROMLocation(const QString &path)
{
    CEmu_settings->setValue("ROM_INFO/location", path);
}

QString CEmuSettings::getROMLocation()
{
    return(CEmu_settings->value("ROM_INFO/location").toString());
}
