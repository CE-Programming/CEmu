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

SendingHandler::SendingHandler(QProgressBar *bar, QTableWidget *t) {
    progress = bar;
    table = t;
}

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

void SendingHandler::resendSelected() {
    QStringList files;

    for (int i = 0; i < table->rowCount(); i++) {
        if (table->item(i, 0)->checkState() == Qt::Checked) {
            files.append(table->item(i, 0)->text());
        }
    }

    sendFiles(files, LINK_FILE);
}

bool SendingHandler::dragOccured(QDragEnterEvent *e) {
    if (isSending || isReceiving || inDebugger) {
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

void SendingHandler::sendFiles(QStringList fileNames, unsigned int location) {
    if (progress) { progress->setVisible(true); }

    if (isSending || isReceiving || inDebugger) {
        QMessageBox::warning(nullptr, QObject::tr("Failed Transfer"), QObject::tr("Transfer failed: Emulation Paused"));
        if (progress) { progress->setVisible(false); }
        return;
    }

    emu_thread->setSendState(true);
    const int fileNum = fileNames.size();

    if (fileNames.isEmpty()) {
        emu_thread->setSendState(false);
        if (progress) { progress->setVisible(false); }
        return;
    }

    /* Wait for an open link */
    unsigned int tries_cnt = 0;
    emu_thread->waitForLink = true;
    do {
        guiDelay(100);
        tries_cnt++;
    } while (emu_thread->waitForLink && tries_cnt < 50);

    if (emu_thread->waitForLink) {
        emu_thread->setSendState(false);
        QMessageBox::warning(Q_NULLPTR, QObject::tr("Failed Transfer"), QObject::tr("Couldn't start the transfer. Make sure the calc is ready (at the home screen, for instance)."));
        if (progress) { progress->setVisible(false); }
        return;
    }

    if (progress) { progress->setMaximum(fileNum); }


    for (int i = 0; i < fileNum; i++) {
        int currRow = table->rowCount();
        bool add = true;

        if (!sendVariableLink(fileNames.at(i).toUtf8(), location)) {
            QMessageBox::warning(Q_NULLPTR, QObject::tr("Failed Transfer"), QObject::tr("A failure occured during transfer of: ")+fileNames.at(i));
        }

        for (int j = 0; j < currRow; j++) {
            if (!table->item(j, 0)->text().compare(fileNames.at(i))) {
                add = false;
            }
        }

        if (add) {
            table->setRowCount(currRow + 1);
            QTableWidgetItem *path = new QTableWidgetItem(fileNames.at(i));
            table->setItem(currRow, 0, path);

            path->setCheckState(Qt::Checked);
        }

        if (progress) { progress->setValue(i); }
        QApplication::processEvents();
    }

    emu_thread->setSendState(false);
    if (progress) { progress->setValue(fileNum); }
    while (isSending);
    if (progress) { progress->setVisible(false); progress->setValue(0); }
}
