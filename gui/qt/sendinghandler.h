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

class SendingHandler : public QObject {
    Q_OBJECT

public:
    explicit SendingHandler(QObject *p = Q_NULLPTR, QProgressBar *bar = Q_NULLPTR, QTableWidget *t = Q_NULLPTR);
    ~SendingHandler() = default;

    void sendFiles(const QStringList &fileNames, unsigned int location);
    bool dragOccured(QDragEnterEvent *e);
    void dropOccured(QDropEvent *e, unsigned int location);
    void resendSelected();
    void addFile(QString &path, bool select);
    void setLoadEquates(bool state);

public slots:
    void sentFile(const QString &file, int ok);
    void resendPressed();
    void removeRow();

signals:
    void send(const QStringList &fileNames, unsigned int location);
    void loadEquateFile(const QString &path);

private:
    void checkDirForEquateFiles(QString &dirPath);
    QStringList getValidFilesFromArchive(const QString &archivePath);

    enum recentIndex {
        RECENT_SELECT=0,
        RECENT_RESEND,
        RECENT_REMOVE,
        RECENT_PATH
    };

    QTemporaryDir m_tempDir;
    QProgressBar *m_progressBar;
    QTableWidget *m_table;
    QIcon m_sendIcon;
    QStringList m_dirs;
    bool m_sendEquates = false;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
