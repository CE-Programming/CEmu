#include "chunks.h"

#define NORMAL 0
#define HIGHLIGHTED 1

#define BUFFER_SIZE 0x10000
#define CHUNK_SIZE 0x1000
#define READ_CHUNK_MASK Q_INT64_C(0xfffffffffffff000)

/* Constructors and file settings */
Chunks::Chunks() {
    QBuffer *buf = new QBuffer();
    setIODevice(*buf);
}

Chunks::Chunks(QIODevice &ioDevice) {
    setIODevice(ioDevice);
}

bool Chunks::setIODevice(QIODevice &ioDevice) {
    _ioDevice = &ioDevice;

    bool ok = _ioDevice->open(QIODevice::ReadOnly);

    if (ok) {  // Try to open IODevice
        _size = _ioDevice->size();
        _ioDevice->close();
    } else {
        QBuffer *buf = new QBuffer();
        _ioDevice = buf;
        _size = 0;
    }

    _chunks.clear();
    _pos = 0;
    return ok;
}


/* Getting data out of Chunks */
QByteArray Chunks::data(qint64 posa, qint64 maxSize, QByteArray *highlighted) {
    qint64 ioDelta = 0;
    int chunkIdx = 0;

    Chunk chunk;
    QByteArray buffer;

    // Do some checks and some arrangements
    if (highlighted) {
        highlighted->clear();
    }

    if (posa >= _size) {
        return buffer;
    }

    if (maxSize < 0) {
        maxSize = _size;
    } else {
        if ((posa + maxSize) > _size) {
            maxSize = _size - posa;
        }
    }

    _ioDevice->open(QIODevice::ReadOnly);

    while (maxSize > 0) {
        chunk.absPos = LLONG_MAX;
        bool chunksLoopOngoing = true;
        while ((chunkIdx < _chunks.count()) && chunksLoopOngoing) {

            // In this section, we track changes before our required data and
            // we take the editdet data, if availible. ioDelta is a difference
            // counter to justify the read pointer to the original data, if
            // data in between was deleted or inserted.

            chunk = _chunks[chunkIdx];
            if (chunk.absPos > posa) {
                chunksLoopOngoing = false;
            } else {
                chunkIdx += 1;
                qint64 count;
                qint64 chunkOfs = posa - chunk.absPos;

                if (maxSize > ((qint64)chunk.data.size() - chunkOfs)) {
                    count = (qint64)chunk.data.size() - chunkOfs;
                    ioDelta += CHUNK_SIZE - chunk.data.size();
                } else {
                    count = maxSize;
                }

                if (count > 0) {
                    buffer += chunk.data.mid(chunkOfs, (int)count);
                    maxSize -= count;
                    posa += count;
                    if (highlighted)
                        *highlighted += chunk.dataChanged.mid(chunkOfs, (int)count);
                }
            }
        }

        if ((maxSize > 0) && (posa < chunk.absPos)) {
            // In this section, we read data from the original source. This only will
            // happen, whe no copied data is available

            qint64 byteCount;
            QByteArray readBuffer;

            if ((chunk.absPos - posa) > maxSize) {
                byteCount = maxSize;
            } else {
                byteCount = chunk.absPos - posa;
            }

            maxSize -= byteCount;
            _ioDevice->seek(posa + ioDelta);
            readBuffer = _ioDevice->read(byteCount);
            buffer += readBuffer;
            if (highlighted)
                *highlighted += QByteArray(readBuffer.size(), NORMAL);
            posa += readBuffer.size();
        }
    }
    _ioDevice->close();
    return buffer;
}

bool Chunks::write(QIODevice &iODevice, qint64 posa, qint64 count) {
    if (count == -1) {
        count = _size;
    }

    bool ok = iODevice.open(QIODevice::WriteOnly);

    if (ok) {
        for (qint64 idx=posa; idx < count; idx += BUFFER_SIZE) {
            QByteArray ba = data(idx, BUFFER_SIZE);
            iODevice.write(ba);
        }
        iODevice.close();
    }

    return ok;
}


/* Set and get highlighting infos */
void Chunks::setDataChanged(qint64 posa, bool dataChanged_) {
    if ((posa < 0) || (posa >= _size)) {
        return;
    }

    int chunkIdx = getChunkIndex(posa);
    qint64 posaInBa = posa - _chunks[chunkIdx].absPos;
    _chunks[chunkIdx].dataChanged[(int)posaInBa] = char(dataChanged_);
}

bool Chunks::dataChanged(qint64 posa) {
    QByteArray highlighted;
    data(posa, 1, &highlighted);

    return bool(highlighted.at(0));
}


/* Search API */
qint64 Chunks::indexOf(const QByteArray &ba, qint64 from) {
    qint64 result = -1;
    QByteArray buffer;

    for (qint64 posa=from; (posa < _size) && (result < 0); posa += BUFFER_SIZE) {
        buffer = data(posa, BUFFER_SIZE + ba.size() - 1);
        int findposa = buffer.indexOf(ba);
        if (findposa >= 0) {
            result = posa + (qint64)findposa;
        }
    }
    return result;
}

qint64 Chunks::lastIndexOf(const QByteArray &ba, qint64 from) {
    qint64 result = -1;
    QByteArray buffer;

    for (qint64 posa=from; (posa > 0) && (result < 0); posa -= BUFFER_SIZE) {
        qint64 sposa = posa - BUFFER_SIZE - (qint64)ba.size() + 1;
        if (sposa < 0) {
            sposa = 0;
        }
        buffer = data(sposa, posa - sposa);
        int findposa = buffer.lastIndexOf(ba);
        if (findposa >= 0) {
            result = sposa + (qint64)findposa;
        }
    }
    return result;
}


/* Char manipulations */
bool Chunks::insert(qint64 posa, char b) {
    if ((posa < 0) || (posa > _size))
        return false;
    int chunkIdx;
    if (posa == _size)
        chunkIdx = getChunkIndex(posa-1);
    else
        chunkIdx = getChunkIndex(posa);
    qint64 posaInBa = posa - _chunks[chunkIdx].absPos;
    _chunks[chunkIdx].data.insert(posaInBa, b);
    _chunks[chunkIdx].dataChanged.insert(posaInBa, char(1));
    for (int idx=chunkIdx+1; idx < _chunks.size(); idx++)
        _chunks[idx].absPos += 1;
    _size += 1;
    _pos = posa;
    return true;
}

bool Chunks::overwrite(qint64 posa, char b) {
    if ((posa < 0) || (posa >= _size))
        return false;
    int chunkIdx = getChunkIndex(posa);
    qint64 posaInBa = posa - _chunks[chunkIdx].absPos;
    _chunks[chunkIdx].data[(int)posaInBa] = b;
    _chunks[chunkIdx].dataChanged[(int)posaInBa] = char(1);
    _pos = posa;
    return true;
}

bool Chunks::removeAt(qint64 posa) {
    if ((posa < 0) || (posa >= _size))
        return false;
    int chunkIdx = getChunkIndex(posa);
    qint64 posaInBa = posa - _chunks[chunkIdx].absPos;
    _chunks[chunkIdx].data.remove(posaInBa, 1);
    _chunks[chunkIdx].dataChanged.remove(posaInBa, 1);
    for (int idx=chunkIdx+1; idx < _chunks.size(); idx++)
        _chunks[idx].absPos -= 1;
    _size -= 1;
    _pos = posa;
    return true;
}


/* Utility functions */
char Chunks::operator[](qint64 posa) {
    return data(posa, 1)[0];
}

qint64 Chunks::pos() {
    return _pos;
}

qint64 Chunks::size() {
    return _size;
}

int Chunks::getChunkIndex(qint64 absposa) {

    // This routine checks, if there is already a copied chunk available. If os, it
    // returns a reference to it. If there is no copied chunk available, original
    // data will be copied into a new chunk.

    int foundIdx = -1;
    int insertIdx = 0;
    qint64 ioDelta = 0;

    for (int idx=0; idx < _chunks.size(); idx++) {
        Chunk chunk = _chunks[idx];
        if ((absposa >= chunk.absPos) && (absposa < (chunk.absPos + chunk.data.size()))) {
            foundIdx = idx;
            break;
        }
        if (absposa < chunk.absPos) {
            insertIdx = idx;
            break;
        }
        ioDelta += chunk.data.size() - CHUNK_SIZE;
        insertIdx = idx + 1;
    }

    if (foundIdx == -1) {
        Chunk newChunk;
        qint64 readAbsposa = absposa - ioDelta;
        qint64 readposa = (readAbsposa & READ_CHUNK_MASK);
        _ioDevice->open(QIODevice::ReadOnly);
        _ioDevice->seek(readposa);
        newChunk.data = _ioDevice->read(CHUNK_SIZE);
        _ioDevice->close();
        newChunk.absPos = absposa - (readAbsposa - readposa);
        newChunk.dataChanged = QByteArray(newChunk.data.size(), char(0));
        _chunks.insert(insertIdx, newChunk);
        foundIdx = insertIdx;
    }
    return foundIdx;
}
