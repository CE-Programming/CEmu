#include "sendinghandler.h"
#include "utils.h"
#include "emuthread.h"
#include "../../core/link.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolButton>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>

#include "QArchive.hpp"

SendingHandler *sendingHandler = Q_NULLPTR;

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
                                            QStringLiteral("8cg"),
                                            QStringLiteral("8ci") };

static inline bool pathHasBundleExtension(const QString& filepath) {
    return filepath.endsWith("b84") || filepath.endsWith("b83");
}

QStringList SendingHandler::getValidFilesFromArchive(const QString& archivePath) {
    if (!m_tempDir.isValid()) {
        QMessageBox::critical(Q_NULLPTR, tr("Transfer error"), tr("Could not create the temporary directory to extract the archive.\nFile: ") + archivePath);
        return {};
    }

    QStringList filesInArchive = QArchive::Reader(archivePath).start().waitForFinished().getFilesList().keys();
    QStringList filesToExtract;
    for (const QString& name : filesInArchive) {
        if (valid_suffixes.contains(QFileInfo(name).suffix().toLower())) {
            filesToExtract.append(name);
        }
    }

    if (filesToExtract.empty()) {
        QMessageBox::critical(Q_NULLPTR, tr("Transfer error"), tr("No valid file found in this archive.\nFile: ") + archivePath);
        return {};
    }

    const QString tempDirPath = m_tempDir.path();

    QArchive::Extractor(archivePath, tempDirPath).onlyExtract(filesToExtract).start().waitForFinished();

    QFileInfoList extractedFilesInfos = QDir(tempDirPath).entryInfoList(QDir::Filter::Files);

    if (extractedFilesInfos.size() != filesToExtract.size()) {
        QMessageBox::critical(Q_NULLPTR, tr("Transfer error"), tr("An error occured while extracting this archive.\nFile: ") + archivePath);
        return {};
    }

    QStringList validFilesPaths;
    for (const QFileInfo& fi : extractedFilesInfos) {
        validFilesPaths.append(fi.absoluteFilePath());
    }
    return validFilesPaths;
}

SendingHandler::SendingHandler(QObject *parent, QProgressBar *bar, QTableWidget *table) : QObject{parent} {
    m_progressBar = bar;
    m_table = table;

    bar->setMinimum(0);
    bar->setMinimumWidth(0);
    bar->setMaximumWidth(200);
    bar->setTextVisible(false);
    bar->setValue(0);
    bar->setVisible(false);
    m_sendIcon.addPixmap(QPixmap(QStringLiteral(":/icons/resources/icons/variables.png")));
}

void SendingHandler::dropOccured(QDropEvent *e, unsigned int location) {
    if (guiSend || guiReceive || guiDebug) {
        e->ignore();
        return;
    }

    const QMimeData* mime_data = e->mimeData();
    if (!mime_data->hasUrls()) {
        return;
    }

    QStringList files;
    for(auto &&url : mime_data->urls()) {
        QString filePath = url.toLocalFile();
        if (pathHasBundleExtension(filePath)) {
            files.append(getValidFilesFromArchive(filePath));
        } else {
            files.append(filePath);
        }
    }

    sendFiles(files, location);
}

void SendingHandler::resendSelected() {
    QStringList files;

    for (int row = 0; row < m_table->rowCount(); row++) {
        if (m_table->item(row, RECENT_SELECT)->checkState() == Qt::Checked) {
            files.append(m_table->item(row, RECENT_PATH)->text());
        }
    }

    sendFiles(files, LINK_FILE);
}

void SendingHandler::resendPressed() {
    QStringList files;

    for (int row = 0; row < m_table->rowCount(); row++){
        if (sender() == m_table->cellWidget(row, RECENT_RESEND)) {
            files.append(m_table->item(row, RECENT_PATH)->text());
            break;
        }
    }

    sendFiles(files, LINK_FILE);
}

void SendingHandler::removeRow() {
    for (int row = 0; row < m_table->rowCount(); row++){
        if (sender() == m_table->cellWidget(row, RECENT_REMOVE)) {
            m_table->removeRow(row);
            break;
        }
    }
}

bool SendingHandler::dragOccured(QDragEnterEvent *e) {
    if (guiSend || guiReceive || guiDebug) {
        e->ignore();
        return false;
    }

    for (QUrl &url : e->mimeData()->urls()) {
        QFileInfo file(url.fileName());
        QString fileExtension = file.suffix().toLower();
        if (!pathHasBundleExtension(fileExtension) && !valid_suffixes.contains(fileExtension)) {
            e->ignore();
            return false;
        }
    }

    e->accept();
    return true;
}

void SendingHandler::sentFile(const QString &file, int ok) {
    bool add = true;
    int rows = m_table->rowCount();

    // Send null to complete sending
    if (ok != LINK_GOOD || file.isEmpty()) {
        if (ok == LINK_ERR) {
            QMessageBox::critical(Q_NULLPTR, QObject::tr("Transfer error"), QObject::tr("Transfer Error, see console for information.\nFile: ") + file);
        }
        if (ok == LINK_WARN) {
            QMessageBox::warning(Q_NULLPTR, QObject::tr("Transfer warning"), QObject::tr("Transfer Warning, see console for information.\nFile: ") + file);
        }
        m_progressBar->setValue(m_progressBar->maximum());
        guiDelay(100);
        if (m_progressBar) {
            m_progressBar->setVisible(false);
            m_progressBar->setValue(0);
        }
        guiSend = false;
        return;
    }

    // check for sending of equate file
    if (m_sendEquates) {
        QFileInfo fi(file);
        QString directory = fi.absolutePath();
        if (!m_dirs.contains(directory)) {
            m_dirs.append(directory);
            checkDirForEquateFiles(directory);
        }
    }

    for (int j = 0; j < rows; j++) {
        if (!m_table->item(j, RECENT_PATH)->text().compare(file)) {
            add = false;
        }
    }

    if (add) {
        QString path = file;
        addFile(path, true);
    }

    if (m_progressBar) {
        m_progressBar->setValue(m_progressBar->value() + 1);
    }
}


void SendingHandler::addFile(QString &file, bool select) {
    QIcon removeIcon(QPixmap(QStringLiteral(":/icons/resources/icons/exit.png")));
    int rows = m_table->rowCount();

    QToolButton *btnRemove = new QToolButton();
    btnRemove->setIcon(removeIcon);

    m_table->setRowCount(rows + 1);
    QTableWidgetItem *path = new QTableWidgetItem(file);
    QTableWidgetItem *resend = new QTableWidgetItem();
    QTableWidgetItem *remove = new QTableWidgetItem();
    QTableWidgetItem *selected = new QTableWidgetItem();

    selected->setFlags(selected->flags() & ~Qt::ItemIsEditable);
    selected->setFlags(resend->flags() & ~Qt::ItemIsEditable);
    selected->setTextAlignment(Qt::AlignCenter);

    QToolButton *btnResend = new QToolButton();
    btnResend->setIcon(m_sendIcon);
    connect(btnResend, &QToolButton::clicked, this, &SendingHandler::resendPressed);
    connect(btnRemove, &QToolButton::clicked, this, &SendingHandler::removeRow);

    m_table->setItem(rows, RECENT_SELECT, selected);
    m_table->setItem(rows, RECENT_RESEND, resend);
    m_table->setItem(rows, RECENT_REMOVE, remove);
    m_table->setItem(rows, RECENT_PATH, path);
    m_table->setCellWidget(rows, RECENT_RESEND, btnResend);
    m_table->setCellWidget(rows, RECENT_REMOVE, btnRemove);

    selected->setCheckState(select ? Qt::Checked : Qt::Unchecked);
}

void SendingHandler::sendFiles(const QStringList &fileNames, unsigned int location) {
    const int fileNum = fileNames.size();

    if (guiSend || guiReceive || guiDebug || !fileNum) {
        return;
    }

    guiSend = true;
    m_dirs.clear();

    if (m_progressBar) {
        m_progressBar->setVisible(true);
        m_progressBar->setMaximum(fileNum);
    }

    emit send(fileNames, location);
}

void SendingHandler::setLoadEquates(bool state) {
    m_sendEquates = state;
}

void SendingHandler::checkDirForEquateFiles(QString &dirPath) {
    QDirIterator dirIt(dirPath, QDirIterator::NoIteratorFlags);
    while (dirIt.hasNext()) {
        dirIt.next();
        QString dirItFile = dirIt.filePath();
        if (QFileInfo(dirItFile).isFile()) {
            QString suffix = QFileInfo(dirItFile).suffix();
            if (suffix == QStringLiteral("map") ||
                suffix == QStringLiteral("inc") ||
                suffix == QStringLiteral("lab")) {
                emit loadEquateFile(dirItFile);
            }
        }
    }
}
