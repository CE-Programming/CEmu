#ifndef SENDINGHANDLER_H
#define SENDINGHANDLER_H

#include <QtWidgets/QProgressBar>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QObject>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>

class SendingHandler : public QObject {
    Q_OBJECT

public:
    explicit SendingHandler(QObject *p = Q_NULLPTR, QProgressBar *bar = Q_NULLPTR, QTableWidget *t = Q_NULLPTR);
    ~SendingHandler() = default;

    void sendFiles(const QStringList& fileNames, unsigned int location);
    bool dragOccured(QDragEnterEvent* e);
    void dropOccured(QDropEvent* e, unsigned int location);
    void resendSelected();
    void addFile(QString &path, bool select);

public slots:
    void sentFile(const QString &file, bool ok);
    void resendPressed();

signals:
    void send(const QStringList& fileNames, unsigned int location);

private:

    enum recentIndex {
        RECENT_SELECT=0,
        RECENT_RESEND,
        RECENT_PATH,
    };

    QProgressBar *progress;
    QTableWidget *table;
    QIcon sendIcon;
};

// Used as global
extern SendingHandler *sendingHandler;

#endif
