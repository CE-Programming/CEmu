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

signals:
    void send(const QStringList &fileNames, unsigned int location);
    void loadEquateFile(const QString &path);

private:
    void checkDirForEquateFiles(QString &dirPath);

    enum recentIndex {
        RECENT_SELECT=0,
        RECENT_RESEND,
        RECENT_PATH,
    };

    QProgressBar *progress;
    QTableWidget *table;
    QIcon sendIcon;
    QStringList directories;
    bool sendEquates = false;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
