#include <QtGui/QPainter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>

#include "utils.h"
#include "profilewidget.h"
#include "keypad/qtkeypadbridge.h"
#include "mainwindow.h"
#include "../../core/lcd.h"

ProfileWidget::ProfileWidget(QWidget *p) : QWidget(p) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ProfileWidget::customContextMenuRequested, this, &ProfileWidget::contextMenu);

    image = QImage(width, height, QImage::Format_RGBX8888);
}

ProfileWidget::~ProfileWidget() {

}


void ProfileWidget::paintEvent(QPaintEvent*) {
    if (!guiEmuValid) {
        return;
    }

    QPainter c(this);
    const QRect& cw = c.window();

    c.fillRect(cw, Qt::white);
}

QString ProfileWidget::setConfig(const QString &config) {
    QStringList setup;
    QStringList string = config.split(',');
    QRegExp hexReg("^[0-9A-F]{6}$", Qt::CaseInsensitive);
    bool foundStart = false;

    foreach (QString str, string) {
        str = str.toLower();
        if (str.contains('x')) {
            QStringList wh = str.split('x');
            if (wh.size() == 2) {
                width = wh.at(0).toUInt();
                height = wh.at(1).toUInt();
            }
        }
        if (str.length() == 8 && str.at(0) == '0' && str.at(1) == 'x') {
            str.remove(0, 2);
        }
        if (str.length() == 7 && str.at(0) == '$') {
            str.remove(0, 1);
        }
        if (hexReg.exactMatch(str)) {
            if (!foundStart) {
                base = str.toUInt(Q_NULLPTR, 16);
                foundStart = true;
            } else {
                top = str.toUInt(Q_NULLPTR, 16);
            }
        }
    }

    setFixedSize(width, height);

    setup.clear();
    setup.append(int2hex(base, 6).toLower());
    setup.append(int2hex(top, 6).toLower());
    setup.append(QString::number(width) + "x" + QString::number(height));
    return setup.join(",");
}

void ProfileWidget::contextMenu(const QPoint &posa) {

}
