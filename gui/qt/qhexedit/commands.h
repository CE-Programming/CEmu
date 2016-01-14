#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoStack>

#include "chunks.h"

class UndoStack : public QUndoStack {
    Q_OBJECT

public:
    UndoStack(Chunks *chunks, QObject * parent=0);
    void insert(qint64 pos, char c);
    void insert(qint64 pos, const QByteArray &ba);
    void removeAt(qint64 pos, qint64 len=1);
    void overwrite(qint64 pos, char c);
    void overwrite(qint64 pos, int len, const QByteArray &ba);

private:
    Chunks * _chunks;
    QObject * _parent;
};

#endif
