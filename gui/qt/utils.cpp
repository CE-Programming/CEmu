#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QCoreApplication>

#include "utils.h"
#include "../../core/os/os.h"
#include "tivarslib/autoloader.h"

bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (FILE *file = fopen_utf8(path.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

uint8_t int2Bpp(int in) {
    uint8_t bpp;
    switch(in) {
        case 1:
            bpp = 0; break;
        case 2:
            bpp = 1; break;
        case 4:
            bpp = 2; break;
        case 8:
            bpp = 3; break;
        case 24:
            bpp = 5; break;
        case 16:
            bpp = 6; break;
        case 12:
            bpp = 7; break;
        default:
            bpp = 6;
            break;
    }
    return bpp;
}

QString bpp2Str(uint8_t bpp) {
    QString str;
    switch(bpp) {
        case 0:
            str = "01"; break;
        case 1:
            str = "02"; break;
        case 2:
            str = "04"; break;
        case 3:
            str = "08"; break;
        case 4:
            str = "16"; break;
        case 5:
            str = "24"; break;
        case 6:
            str = "16"; break;
        case 7:
            str = "12"; break;
    }
    return str;
}

int hex2int(QString str) {
    return (int)strtol(str.toStdString().c_str(), nullptr, 16);
}

QString int2hex(uint32_t a, uint8_t l) {
    return QString::number(a, 16).rightJustified(l, '0', true).toUpper();
}

std::string calc_var_content_string(const calc_var_t& var)
{
    auto func = tivars::TypeHandlerFuncGetter::getStringFromDataFunc((int)var.type);
    return func(data_t(var.data, var.data + var.size), options_t());
}

void guiDelay(int ms) {
    QTime dt = QTime::currentTime().addMSecs(ms);
    while (QTime::currentTime() < dt) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}
