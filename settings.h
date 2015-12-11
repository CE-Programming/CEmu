#ifndef CEMUSETTINGS_H
#define CEMUSETTINGS_H

class QSettings;

class CEmuSettings
{
public:
    static CEmuSettings* Instance();
    void setROMLocation(const QString &path);
    QString getROMLocation();

private:
    CEmuSettings();
    ~CEmuSettings();
    CEmuSettings(CEmuSettings const&){};             // copy constructor is private
    QSettings *CEmu_settings;
    static CEmuSettings* m_pInstance;

signals:

public slots:
};

#endif // CEMUSETTINGS_H
