#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtWidgets/QProgressBar>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QTemporaryDir>

#include "../../core/emu.h"
#include "../../core/link.h"

class SendingHandler : public QObject {
    Q_OBJECT

public:
    explicit SendingHandler(QObject *p = Q_NULLPTR, QProgressBar *bar = Q_NULLPTR, QTableWidget *t = Q_NULLPTR);
    ~SendingHandler() = default;

    void sendFiles(const QStringList &fileNames, int location);
    bool dragOccured(QDragEnterEvent *e);
    void dropOccured(QDropEvent *e, int location);
    void resendSelected();
    void addFile(const QString &path, bool select);
    void setLoadEquates(bool state);

public slots:
    void linkProgress(int amount, int total);
    void sentFile(const QString &file, int ok);
    void resendPressed();
    void removeRow();

signals:
    void send(const QStringList &names, int location);
    void loadEquateFile(const QString &path);

private:
    void checkDirForEquateFiles(QString &dirPath);
    QStringList getValidFilesFromArchive(const QString &archivePath);

    enum {
        RECENT_REMOVE_COL,
        RECENT_RESEND_COL,
        RECENT_SELECT_COL,
        RECENT_PATH_COL
    };

    QTemporaryDir m_tempDir;
    QProgressBar *m_progressBar;
    QTableWidget *m_table;
    QIcon m_iconSend;
    QIcon m_iconCheck, m_iconCheckGray;
    QStringList m_dirs;
    bool m_sendEquates = false;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
