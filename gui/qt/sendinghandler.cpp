#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include "sendinghandler.h"

#include "utils.h"
#include "emuthread.h"
#include "../../core/link.h"

SendingHandler sendingHandler;

SendingHandler::SendingHandler() {}

void SendingHandler::dropOccured(QDropEvent *e, unsigned int location) {
    if (isSending || isReceiving || inDebugger) {
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

bool SendingHandler::dragOccured(QDragEnterEvent *e) {
    if (isSending || isReceiving || inDebugger) {
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
        if (!valid_suffixes.contains(file.suffix().toLower())) {
            e->ignore();
            return false;
        }
    }

    e->accept();
    return true;
}

void SendingHandler::sendFiles(QStringList fileNames, unsigned location) {
    if (isSending || isReceiving || inDebugger) {
        QMessageBox::warning(nullptr, QObject::tr("Failed Transfer"), QObject::tr("Transfer failed: Emulation Paused"));
        return;
    }

    emu_thread->setSendState(true);
    const int fileNum = fileNames.size();

    if (fileNames.isEmpty()) {
        emu_thread->setSendState(false);
        return;
    }

    /* Wait for an open link */
    unsigned int tries_cnt = 0;
    emu_thread->waitForLink = true;
    do {
        guiDelay(50);
        tries_cnt++;
    } while (emu_thread->waitForLink && tries_cnt < 50);

    if (emu_thread->waitForLink) {
        emu_thread->setSendState(false);
        QMessageBox::warning(nullptr, QObject::tr("Failed Transfer"), QObject::tr("Couldn't start the transfer. Make sure the calc is ready (at the home screen, for instance)."));
        return;
    }

    QProgressDialog *progress = new QProgressDialog("Sending Files...", QString(), 0, fileNum, Q_NULLPTR);
    progress->setWindowModality(Qt::WindowModal);

    progress->show();

    for (int i = 0; i < fileNum; i++) {
        if (!sendVariableLink(fileNames.at(i).toUtf8(), location)) {
            QMessageBox::warning(Q_NULLPTR, QObject::tr("Failed Transfer"), QObject::tr("A failure occured during transfer of: ")+fileNames.at(i));
        }
        progress->setLabelText(fileNames.at(i).toUtf8());
        progress->setValue(progress->value() + 1);
        QApplication::processEvents();
    }

    progress->setValue(progress->value() + 1);
    emu_thread->setSendState(false);
    guiDelay(200);

    delete progress;
}
