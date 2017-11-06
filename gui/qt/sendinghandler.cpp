#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include "sendinghandler.h"

#include "utils.h"
#include "emuthread.h"
#include "../../core/link.h"

SendingHandler *sendingHandler = Q_NULLPTR;

SendingHandler::SendingHandler(QObject *p, QProgressBar *bar, QTableWidget *t) : QObject(p) {
    progress = bar;
    table = t;
    connect(this, &SendingHandler::send, emu_thread, &EmuThread::send, Qt::QueuedConnection);
    connect(emu_thread, &EmuThread::sentFile, this, &SendingHandler::sentFile, Qt::QueuedConnection);
}

SendingHandler::~SendingHandler() {
}

void SendingHandler::dropOccured(QDropEvent *e, unsigned int location) {
    if (guiSend || guiReceive || guiDebug) {
        return e->ignore();
    }

    const QMimeData* mime_data = e->mimeData();
    if (!mime_data->hasUrls()) {
        return;
    }

    QStringList files;
    for(auto &&url : mime_data->urls()) {
        files.append(url.toLocalFile());
    }

    sendFiles(files, location);
}

void SendingHandler::resendSelected() {
    QStringList files;

    for (int row = 0; row < table->rowCount(); row++) {
        if (table->item(row, 0)->checkState() == Qt::Checked) {
            files.append(table->item(row, 1)->text());
        }
    }

    sendFiles(files, LINK_FILE);
}

bool SendingHandler::dragOccured(QDragEnterEvent *e) {
    if (guiSend || guiReceive || guiDebug) {
        e->ignore();
        return false;
    }

    for (QUrl &url : e->mimeData()->urls()) {
        static const QStringList valid_suffixes = { QStringLiteral("8xp"),
                                                    QStringLiteral("8xv"),
                                                    QStringLiteral("8xl"),
                                                    QStringLiteral("8xn"),
                                                    QStringLiteral("8xm"),
                                                    QStringLiteral("8xy"),
                                                    QStringLiteral("8xg"),
                                                    QStringLiteral("8xs"),
                                                    QStringLiteral("8xd"),
                                                    QStringLiteral("8xw"),
                                                    QStringLiteral("8xc"),
                                                    QStringLiteral("8xz"),
                                                    QStringLiteral("8xt"),
                                                    QStringLiteral("8ca"),
                                                    QStringLiteral("8ci") };

        QFileInfo file(url.fileName());
        if (!valid_suffixes.contains(file.suffix().toLower())) {
            e->ignore();
            return false;
        }
    }

    e->accept();
    return true;
}

void SendingHandler::sentFile(const QString &file, bool ok) {
    bool add = true;
    int rows = table->rowCount();

    // Send null to complete sending
    if (!ok || file.isEmpty()) {
        if (!ok) {
            QMessageBox::critical(Q_NULLPTR, QObject::tr("Failed Transfer"), QObject::tr("File: ")+file);
        }
        progress->setValue(progress->maximum());
        guiDelay(100);
        if (progress) {
            progress->setVisible(false);
            progress->setValue(0);
        }
        guiSend = false;
        return;
    }

    for (int j = 0; j < rows; j++) {
        if (!table->item(j, 1)->text().compare(file)) {
            add = false;
        }
    }

    if (add) {
        table->setRowCount(rows + 1);
        QTableWidgetItem *path = new QTableWidgetItem(file);
        QTableWidgetItem *selected = new QTableWidgetItem();

        table->setItem(rows, 0, selected);
        table->setItem(rows, 1, path);

        selected->setCheckState(Qt::Checked);
    }

    if (progress) {
        progress->setValue(progress->value()+1);
    }
}

void SendingHandler::sendFiles(const QStringList &fileNames, unsigned int location) {
    const int fileNum = fileNames.size();

    if (guiSend || guiReceive || guiDebug || !fileNum) {
        return;
    }

    guiSend = true;

    if (progress) {
        progress->setVisible(true);
        progress->setMaximum(fileNum);
    }

    emit send(fileNames, location);
}
