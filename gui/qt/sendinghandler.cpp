#include "sendinghandler.h"
#include "utils.h"
#include "emuthread.h"
#include "../../core/link.h"
#include "archive/extractor.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolButton>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>

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
                                            QStringLiteral("8ci"), };

static inline bool pathHasBundleExtension(const QString& filepath) {
    return filepath.endsWith("b84", Qt::CaseInsensitive) || filepath.endsWith("b83", Qt::CaseInsensitive);
}

static QStringList bundleList;
static bool checkValidFile(const char *path) {
    QString npath(path);
    foreach (const QString &str, valid_suffixes) {
        if (npath.endsWith(str, Qt::CaseInsensitive)) {
            bundleList.append(path);
            return true;
        }
    }
    return false;
}

QStringList SendingHandler::getValidFilesFromArchive(const QString& archivePath) {
    if (!m_tempDir.isValid()) {
        QMessageBox::critical(Q_NULLPTR, tr("Transfer error"), tr("Could not create the temporary directory to extract the archive.\nFile: ") + archivePath);
        return {};
    }

    bundleList.clear();
    extractor(archivePath.toStdString().c_str(), m_tempDir.path().toStdString().c_str(), checkValidFile);
    return bundleList;
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
    m_iconSend.addPixmap(QPixmap(":/icons/resources/icons/variables.png"));
    m_iconCheck.addPixmap(QPixmap(":/icons/resources/icons/check.png"));
    m_iconCheckGray.addPixmap(QPixmap(":/icons/resources/icons/checkgray.png"));
}

void SendingHandler::dropOccured(QDropEvent *e, int location) {
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
        const QString filePath = url.toLocalFile();
        files.append(filePath);
    }

    sendFiles(files, location);
}

void SendingHandler::resendSelected() {
    QStringList files;

    for (int row = 0; row < m_table->rowCount(); row++) {
        if (static_cast<QAbstractButton *>(m_table->cellWidget(row, RECENT_SELECT_COL))->isChecked()) {
            files.append(m_table->item(row, RECENT_PATH_COL)->text());
        }
    }

    sendFiles(files, LINK_FILE);
}

void SendingHandler::resendPressed() {
    QStringList files;

    for (int row = 0; row < m_table->rowCount(); row++){
        if (sender() == m_table->cellWidget(row, RECENT_RESEND_COL)) {
            files.append(m_table->item(row, RECENT_PATH_COL)->text());
            break;
        }
    }

    sendFiles(files, LINK_FILE);
}

void SendingHandler::removeRow() {
    for (int row = 0; row < m_table->rowCount(); row++){
        if (sender() == m_table->cellWidget(row, RECENT_REMOVE_COL)) {
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

    static int result = LINK_GOOD;
    result |= ok;

    // Send null to complete sending
    if (file.isEmpty()) {
        if (result & LINK_ERR) {
            QMessageBox::critical(Q_NULLPTR, QObject::tr("Transfer error"), QObject::tr("Transfer Error, see console for information."));
        }
        else if (result & LINK_WARN) {
            QMessageBox::warning(Q_NULLPTR, QObject::tr("Transfer warning"), QObject::tr("Transfer Warning, see console for information."));
        }
        result = LINK_GOOD;
        m_progressBar->setValue(m_progressBar->maximum());
        guiDelay(100);
        if (m_progressBar) {
            m_progressBar->setVisible(false);
            m_progressBar->setValue(0);
        }
        guiSend = false;
        return;
    }

    QFileInfo fi(file);
    QString directory = fi.absolutePath();

    // check for sending of equate file
    if (m_sendEquates) {
        if (!m_dirs.contains(directory)) {
            m_dirs.append(directory);
            checkDirForEquateFiles(directory);
        }
    }

    // don't add temp files (created by bundles)
    if (directory != m_tempDir.path()) {
        addFile(file, true);
    }

    if (m_progressBar) {
        m_progressBar->setValue(m_progressBar->value() + 1);
    }
}


void SendingHandler::addFile(const QString &file, bool select) {
    int j, rows = m_table->rowCount();

    for (j = 0; j < rows; j++) {
        if (!m_table->item(j, RECENT_PATH_COL)->text().compare(file, Qt::CaseInsensitive)) {
            return;
        }
    }

    m_table->setSortingEnabled(false);

    m_table->setRowCount(rows + 1);
    QTableWidgetItem *remove = new QTableWidgetItem;
    QTableWidgetItem *resend = new QTableWidgetItem;
    QTableWidgetItem *selected = new QTableWidgetItem;
    QTableWidgetItem *path = new QTableWidgetItem(file);

    QIcon removeIcon(QPixmap(QStringLiteral(":/icons/resources/icons/exit.png")));
    QToolButton *btnRemove = new QToolButton;
    btnRemove->setIcon(removeIcon);

    QToolButton *btnResend = new QToolButton;
    btnResend->setIcon(m_iconSend);

    QToolButton *btnSelect = new QToolButton;
    btnSelect->setIcon(select ? m_iconCheck : m_iconCheckGray);
    btnSelect->setCheckable(true);
    btnSelect->setChecked(select);

    connect(btnResend, &QToolButton::clicked, this, &SendingHandler::resendPressed);
    connect(btnRemove, &QToolButton::clicked, this, &SendingHandler::removeRow);
    connect(btnSelect, &QToolButton::clicked, [this, btnSelect](bool checked) { btnSelect->setIcon(checked ? m_iconCheck : m_iconCheckGray); });

    m_table->setItem(rows, RECENT_REMOVE_COL, remove);
    m_table->setItem(rows, RECENT_RESEND_COL, resend);
    m_table->setItem(rows, RECENT_SELECT_COL, selected);
    m_table->setItem(rows, RECENT_PATH_COL, path);
    m_table->setCellWidget(rows, RECENT_REMOVE_COL, btnRemove);
    m_table->setCellWidget(rows, RECENT_RESEND_COL, btnResend);
    m_table->setCellWidget(rows, RECENT_SELECT_COL, btnSelect);

    m_table->setVisible(false);
    m_table->resizeColumnsToContents();
    m_table->setVisible(true);
    m_table->setSortingEnabled(true);
}

void SendingHandler::sendFiles(const QStringList &fileNames, int location) {
    QStringList list = fileNames;

    if (guiSend || guiReceive || guiDebug || !list.size()) {
        return;
    }

    guiSend = true;
    m_dirs.clear();

    foreach(const QString &str, fileNames) {
        if (pathHasBundleExtension(str)) {
            list.removeOne(str);
            list.append(getValidFilesFromArchive(str));
            addFile(str, true);
        }
    }

    if (m_progressBar) {
        m_progressBar->setVisible(true);
        m_progressBar->setMaximum(list.size());
    }

    emit send(list, location);
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
