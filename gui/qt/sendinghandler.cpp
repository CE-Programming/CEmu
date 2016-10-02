#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include "sendinghandler.h"

#include "emuthread.h"
#include "../../core/link.h"

SendingHandler sending_handler;

SendingHandler::SendingHandler() {
}

void SendingHandler::dropOccured(QDropEvent *e, unsigned location) {
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

bool SendingHandler::dragOccured(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls() == false) {
        e->ignore();
        return false;
    }

    for(QUrl &url : e->mimeData()->urls()) {
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
        if(!valid_suffixes.contains(file.suffix().toLower())) {
            e->ignore();
            return false;
        }
    }

    e->accept();
    return true;
}

void SendingHandler::sendFiles(QStringList fileNames, unsigned location) {
    if (inDebugger) {
        return;
    }

    emu_thread->setSendState(true);
    const unsigned int fileNum = fileNames.size();

    if (fileNum == 0) {
        emu_thread->setSendState(false);
        return;
    }

    /* Wait for an open link */
    emu_thread->waitForLink = true;
    do {
        QThread::msleep(50);
    } while(emu_thread->waitForLink);

    QProgressDialog progress("Sending Files...", QString(), 0, fileNum, nullptr);
    progress.setWindowModality(Qt::WindowModal);

    progress.show();
    QApplication::processEvents();

    for (unsigned int i = 0; i < fileNum; i++) {
        if(!sendVariableLink(fileNames.at(i).toUtf8(), location)) {
            QMessageBox::warning(nullptr, QObject::tr("Failed Transfer"), QObject::tr("A failure occured during transfer of: ")+fileNames.at(i));
        }
        progress.setLabelText(fileNames.at(i).toUtf8());
        progress.setValue(progress.value()+1);
        QApplication::processEvents();
    }

    progress.setValue(progress.value()+1);
    QApplication::processEvents();
    QThread::msleep(100);

    emu_thread->setSendState(false);
}
