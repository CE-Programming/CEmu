/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2018, Antony jr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 *  @filename 	 	: QArchive.hpp
 *  @description 	: Namespace and Class Declaration for QArchive.
 * =============================================================================
*/
#if !defined (QARCHIVE_HPP_INCLUDED)
#define QARCHIVE_HPP_INCLUDED

#include <QtCore>
#include <QtConcurrent>
#include <functional>

/*
 * Getting the libarchive headers for the
 * runtime operating system.
 * ======================================
*/
extern "C" {
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
}
// ===

/*
 * To fix build errors on vs.
 * ==========================
 *  Fixed by https://github.com/hcaihao
*/
#if defined(_MSC_VER)
#include <BaseTsd.h>
#include <io.h>
typedef SSIZE_T ssize_t;
#endif
// ====

namespace QArchive   // QARCHIVE NAMESPACE STARTS
{

/*
* QARCHIVE ERROR CODES
* ====================
*/
enum {
    NO_ARCHIVE_ERROR,
    ARCHIVE_QUALITY_ERROR,
    ARCHIVE_READ_ERROR,
    ARCHIVE_UNCAUGHT_ERROR,
    ARCHIVE_FATAL_ERROR,
    ARCHIVE_WRONG_PASSWORD,
    ARCHIVE_PASSWORD_NOT_GIVEN,
    ARCHIVE_WRITE_OPEN_ERROR,
    DISK_OPEN_ERROR,
    DISK_READ_ERROR,
    DESTINATION_NOT_FOUND,
    FILE_NOT_EXIST,
    INVALID_DEST_PATH,
    NOT_ENOUGH_MEMORY,
    FILE_OPEN_FAILED
};
// ===

/*
* SIGNAL CODES FOR 'setFunc' SLOTS
* ================================
*/
enum {
    CANCELED,
    COMPRESSED,
    COMPRESSING,
    EXTRACTED,
    EXTRACTING,
    FINISHED,
    PASSWORD_REQUIRED,
    PROGRESS,
    RESUMED,
    STARTED,
    PAUSED
};
// ===

/*
 * Class UNBlock <- Inherits QObject.
 * ==============
 *
 *  This is a advanced for-loop with C++11 features
 *  backed up by Qt5 Framework.
 *
 * ==================================================
 * +This class is private and thus its not intended +
 * +to be used by the user.                         +
 * ==================================================
*/
class UNBlock : public QObject
{
    Q_OBJECT
public:
    explicit UNBlock(QObject *parent = nullptr);
    explicit UNBlock(
        std::function<int(void)> initializer,
        std::function<int(void)> condition,
        std::function<void(void)> expression,
        std::function<int(void)> block,
        std::function<void(int)> deinitializer,
        int endpoint,
        int TIterations = 0);

    UNBlock &setInitializer(std::function<int(void)>);
    UNBlock &setCondition(std::function<int(void)>);
    UNBlock &setExpression(std::function<void(void)>);
    UNBlock &setCodeBlock(std::function<int(void)>);
    UNBlock &setDeInitializer(std::function<void(int)>);
    UNBlock &setEndpoint(int);
    UNBlock &setTotalIterations(int);

    ~UNBlock();
public Q_SLOTS:
    UNBlock &waitForFinished(void);
    UNBlock &start(void);
    UNBlock &cancel(void);
    UNBlock &pause(void);
    UNBlock &resume(void);

    bool isRunning(void) const;
    bool isCanceled(void) const;
    bool isPaused(void) const;
    bool isStarted(void) const;
private Q_SLOTS:
    void loop(void);
    void setStarted(bool);
    void setPaused(bool);
    void setCanceled(bool);
    void setCancelRequested(bool);
    void setPauseRequested(bool);
Q_SIGNALS:
    void started(void);
    void finished(void);
    void paused(void);
    void resumed(void);
    void doResume(void);
    void canceled(void);
    void progress(int);

private:
    bool _bStarted = false,
         _bPaused = false,
         _bCanceled = false,
         _bIsCancelRequested = false,
         _bIsPauseRequested = false;

    std::function<int(void)> initializer;
    std::function<int(void)> condition;
    std::function<void(void)> expression;
    std::function<int(void)> codeBlock;
    std::function<void(int)> deinitializer;
    int endpoint;
    int TIterations = 0;

    QMutex Mutex;
    QFuture<void> *Future = nullptr;
};

/*
 * ===========
*/

/*
 * Class Extractor <- Inherits QObject.
 * ===============
 *
 *  Takes care of extraction of archives with the help
 *  of libarchive.
 *
 *  Constructors:
 *  ============
 *  	Extractor(QObject *parent = NULL) - Constructs an empty Extractor Object.
 *  	Extractor(const QString &Archive) - Constructs and sets Archive(1).
 *  	Extractor(const QString &Archive , const QString &Destination) - Constructs and
 *  	                                                                 sets Archive(1) and Destination(2).
 *
 *  Methods:
 *  =======
 *      setArchive(const QString &Archive) - Sets the Archive(1) Path.
 *      setArchive(const QString &Archive , const QString &Destination) - Sets the Archive(1) and Destination(2) Path.
 *      setPassword(const QString &Password) - Sets the password for the Archive(1).
 *      setAskPassword(bool AskPassword) - If set true , If password is required it will be asked until the user sets the
 *                                         correct password or a empty password.
 *      setBlocksize(int size) - Set the blocksize.
 *      onlyExtract(const QString &MemberInArchive) - Only Extract a Member in the given Archive(1).
 *      onlyExtract(const QStringList &MembersInArchive) - Only Extract a list of Members in the given Archive(1).
 *      clear(void) - Clears the extractor objects cache.
 *
 *  Slots:
 *  =====
 *      waitForFinished(void) - Blocks the caller thread until the extraction is finished or canceled.
 *      start(void) - Starts the extraction process in a different thread.
 *      pause(void) - Pauses the extraction process.
 *      resume(void) - Resumes the extraction process.
 *      cancel(void) - Cancel the extraction process.
 *
 *      isRunning() - Returns True if the extraction process is running.
 *      isCanceled() - Returns True if the extraction process has been canceled.
 *      isPaused() - Returns True if the extraction process is paused.
 *      isStarted() - Returns True if the extraction process is started.
 *
 *      setFunc(short signal , std::function<void(void)> function) - Connects the lambda function to the Signal with
 *                                                                   respect to the given signal(1) code.
 *                                                                   This slot can connect the following signals.
 *                                                                   started,finished,canceled,paused, and resumed.
 *
 *      setFunc(short signal , std::function<void(QString)> function) - Connects the lambda function to extracting or
 *                                                                      extracting signal with respect to the given
 *                                                                      signal(1) code.
 *
 *      setFunc(short signal , std::function<void(int)> function) - Connects the lambda function to progress or
 *                                                                  password required signal with respect
 *                                                                  to the given signal(1) code.
 *
 *      setFunc(std::function<void(short,QString)> function) - Connects the lambda function to the error signal.
 *
 *  Signals:
 *  =======
 *      started(void) - Emitted when the extraction process is started.
 *      finished(void) - Emitted when the extraction process is finished without any error.
 *      canceled(void) - Emitted when the extraction process is canceled.
 *      paused(void) - Emitted when the extraction process is paused successfully.
 *      resumed(void) - Emitted when the extraction process is resumed successfully.
 *      progress(int) - Emitted on progress update.
 *      passwordRequired(int) - Emitted when password is required for the extraction.
 *      error(short errorCode , QString eMsg) - Emitted when something goes wrong.
*/

class Extractor  : public QObject // CLASS EXTRACTOR
{
    Q_OBJECT
public:
    explicit Extractor(QObject *parent = nullptr);
    explicit Extractor(const QString&);
    explicit Extractor(const QString&, const QString&);
    Extractor &setArchive(const QString&);
    Extractor &setArchive(const QString&, const QString&);
#if ARCHIVE_VERSION_NUMBER >= 3003002
    Extractor &setPassword(const QString&);
    Extractor &setAskPassword(bool);
#endif
    Extractor &setBlocksize(int);
    Extractor &onlyExtract(const QString&);
    Extractor &onlyExtract(const QStringList&);
    Extractor &clear(void);
    ~Extractor();

public Q_SLOTS:
    Extractor &waitForFinished(void);
    Extractor &start(void);
    Extractor &pause(void);
    Extractor &resume(void);
    Extractor &cancel(void);

    bool isRunning() const;
    bool isCanceled() const;
    bool isPaused() const;
    bool isStarted() const;

    Extractor &setFunc(short, std::function<void(void)>);
    Extractor &setFunc(short, std::function<void(QString)>);
    Extractor &setFunc(short, std::function<void(int)>);
    Extractor &setFunc(std::function<void(short,QString)>);

Q_SIGNALS:
    void started(void);
    void finished(void);
    void paused(void);
    void resumed(void);
    void canceled(void);
    void progress(int);
#if ARCHIVE_VERSION_NUMBER >= 3003002
    void passwordRequired(int);
    void submitPassword(void);
#endif
    void extracted(const QString&);
    void extracting(const QString&);
    void error(short, const QString&);

private Q_SLOTS:
    int init(void);
    int condition();
    int loopContent(void);
    int totalFileCount(void);
    QString cleanDestPath(const QString& input);
    char *concat(const char *dest, const char *src);

private:
    int ret = 0;
    QSharedPointer<struct archive> archive;
    QSharedPointer<struct archive> ext;
    struct archive_entry *entry;

    QMutex mutex;
#if ARCHIVE_VERSION_NUMBER >= 3003002
    bool AskPassword = false;
    int PasswordTries = 0;
#endif
    int flags = ARCHIVE_EXTRACT_TIME |
                ARCHIVE_EXTRACT_PERM |
                ARCHIVE_EXTRACT_SECURE_NODOTDOT;
    int BlockSize = 10240;
    QString ArchivePath;
    QString Destination;
#if ARCHIVE_VERSION_NUMBER >= 3003002
    QString Password;
#endif
    QStringList OnlyExtract;
    UNBlock *UNBlocker = nullptr;


#if ARCHIVE_VERSION_NUMBER >= 3003002
    /*
     * This callback makes it possible to check if the password is wrong or
     * correct and also loop until the user gives a correct password or an
     * empty password.
     * ====================================================================
    */
    static const char *password_callback(struct archive *a, void *_client_data)
    {
        (void)a; /* UNUSED */
        Extractor *e = (Extractor*)_client_data;
        if(e->AskPassword) {
            if(e->PasswordTries > 0 || e->Password.isEmpty()) {
                QEventLoop Freeze;
                e->connect(e, SIGNAL(submitPassword()), &Freeze, SLOT(quit()));
                QTimer::singleShot(1000, [e]() {
                    if(e->PasswordTries > 0) {
                        e->ret = ARCHIVE_WRONG_PASSWORD;
                        e->error(ARCHIVE_WRONG_PASSWORD, e->ArchivePath);
                    }
                    emit(e->passwordRequired(e->PasswordTries));
                }); // emit signal.
                Freeze.exec();
                e->disconnect(e, SIGNAL(submitPassword()), &Freeze, SLOT(quit()));
                if(e->Password.isEmpty()) {
                    e->ret = ARCHIVE_PASSWORD_NOT_GIVEN;
                    e->error(ARCHIVE_PASSWORD_NOT_GIVEN, e->ArchivePath);
                    return NULL;
                }
            }
        } else {
            if(e->Password.isEmpty()) {
                e->ret = ARCHIVE_PASSWORD_NOT_GIVEN;
                e->error(ARCHIVE_PASSWORD_NOT_GIVEN, e->ArchivePath);
                return NULL;
            } else if(e->PasswordTries > 0) {
                e->ret = ARCHIVE_WRONG_PASSWORD;
                e->error(ARCHIVE_WRONG_PASSWORD, e->ArchivePath);
                return NULL;
            }
        }
        e->PasswordTries += 1;
        return e->Password.toUtf8().constData();
    }
    // ==========
#endif
}; // CLASS EXTRACTOR ENDS

/*
 * SUPPORTED ARCHIVE TYPES FOR COMPRESSOR
 * ======================================
*/
enum {
    NO_FORMAT,
    BZIP,
    BZIP2,
    CPIO,
    GZIP,
    XAR,
    SEVEN_ZIP,
    ZIP
};
// =======


/*
 * Class Compressor <- Inherits QObject.
 * ================
 *
 *  Compresses files and folder into a archive.
 *
 *  Constructors:
 *  ============
 *      Compressor(QObject *parent = NULL) - Constructs an empty Compressor Object.
 *      Compressor(const QString &Archive , const QStringList &files) - Constructs and Sets an Archive(1) with
 *                                                                      the file(s) from Files(2) list.
 *	    Compressor(const QString &Archive , const QString &file) - Constructs and Sets an Archive(1) and add
 *	                                                               a single file(2).
 *  	Compressor(const QString &Archive) - Constructs and Set the Archive(1) path.
 *
 *  Methods:
 *  =======
 *      setArchive(const QString &Archive) - Set the Archive(1) path.
 *      setArchive(const QString &Archive , const QString &file) - Set the Archive(1) path and add a single file(2).
 *      setArchive(const QString &Archive, const QStringList &files) - Set the Archive(1) path and adds all file(s)
 *                                                                     from list
 *      setArchiveFormat(short type) - Sets the Archive type(1) with respect to the format codes.
 *      setPassword(const QString &Password) - Set Password(1) for the current Archive. ( May not work now )
 *      setBlocksize(int size) - Set the blocksize for the compression. ( May not work now )
 *      setCompressionLevel(int level) - Set the compression level for zip , 7zip and rar types.
 *      addFiles(const QString &file) - Add a single file to the archive.
 *      addFiles(const QStringList &files) - Add all file(s) from the list to the archive.
 *      removeFiles(const QString &file) - Removes a single file from the archive.
 *      removeFiles(const QStringList &files) - Removes all file(s) form the list from the archive.
 *      clear(void) - Clears the Objects cache.
 *
 *  Slots:
 *  =====
 *      waitForFinished(void) - Blocks the caller thread until the comression is finished or canceled.
 *      start(void) - Starts the compression process in a different thread.
 *      pause(void) - Pauses the compression process.
 *      resume(void) - Resumes the compression process.
 *      cancel(void) - Cancel the compression process.
 *
 *      isRunning() - Returns True if the compression process is running.
 *      isCanceled() - Returns True if the compression process has been canceled.
 *      isPaused() - Returns True if the compression process is paused.
 *      isStarted() - Returns True if the compression process is started.
 *
 *      setFunc(short signal , std::function<void(void)> function) - Connects the lambda function to the Signal with
 *                                                                   respect to the given signal(1) code.
 *                                                                   This slot can connect the following signals.
 *                                                                   started,finished,canceled,paused, and resumed.
 *
 *      setFunc(short signal , std::function<void(QString)> function) - Connects the lambda function to compressing or
 *                                                                      compressed signal with respect to the given
 *                                                                      signal(1) code.
 *
 *      setFunc(short signal , std::function<void(int)> function) - Connects the lambda function to progress or
 *                                                                  password required signal with respect
 *                                                                  to the given signal(1) code.
 *  Signals:
 *  =======
 *      started(void) - Emitted when the compression process is started.
 *      finished(void) - Emitted when the compression process is finished without any error.
 *      canceled(void) - Emitted when the compression process is canceled.
 *      paused(void) - Emitted when the compression process is paused successfully.
 *      resumed(void) - Emitted when the compression process is resumed successfully.
 *      progress(int) - Emitted on progress update.
 *      error(short errorCode , QString eMsg) - Emitted when something goes wrong.
*/

class Compressor : public QObject // CLASS COMPRESSOR
{
    Q_OBJECT
public:
    explicit Compressor(QObject *parent = nullptr);
    explicit Compressor(const QString& archive);
    explicit Compressor(const QString& archive, const QStringList& files);
    explicit Compressor(const QString& archive, const QString& file);
    Compressor &setArchive(const QString &archive);
    Compressor &setArchive(const QString &archive, const QString &file);
    Compressor &setArchive(const QString &archive, const QStringList &files);
    Compressor &setArchiveFormat(short type);
#if ARCHIVE_VERSION_NUMBER >= 3003002
    Compressor &setPassword(const QString&);
#endif
    Compressor &setBlocksize(int);
    Compressor &setCompressionLevel(int);
    Compressor &addFiles(const QString& file);
    Compressor &addFiles(const QStringList& files);
    Compressor &removeFiles(const QString& file);
    Compressor &removeFiles(const QStringList& files);
    Compressor &clear(void);
    ~Compressor();

public Q_SLOTS:
    Compressor &waitForFinished(void);
    Compressor &start(void);
    Compressor &pause(void);
    Compressor &resume(void);
    Compressor &cancel(void);

    bool isRunning() const;
    bool isCanceled() const;
    bool isPaused() const;
    bool isStarted() const;

    Compressor &setFunc(short, std::function<void(void)>);
    Compressor &setFunc(short, std::function<void(QString)>);
    Compressor &setFunc(std::function<void(int)>);
    Compressor &setFunc(std::function<void(short,QString)>);
private Q_SLOTS:
    int init(void);
    int condition(void);
    void expression(void);
    int loopContent(void);
    void deinit(int);
    QString isDirInFilesList(void);
    void populateDirectory(void);
    void checkNodes(void);
    void getArchiveFormat(void);
Q_SIGNALS:
    void started();
    void finished();
    void paused();
    void resumed();
    void canceled();
    void progress(int);
    void compressing(const QString&);
    void compressed(const QString&);
    void error(short, const QString&);
private:
    int ret = 0;
    QSharedPointer<struct archive> archive;
    QSharedPointer<QSaveFile> tempFile;

    QMutex mutex;
    int BlockSize = 10240;
    int CompressionLevel = 0;
    short archiveFormat = NO_FORMAT;
    QString archivePath;
#if ARCHIVE_VERSION_NUMBER >= 3003002
    QString Password;
#endif
    QMap<QString, QString>::iterator mapIter;
    QMap<QString, QString> nodes;  // (1)-> File path , (2)-> entry in archive.
    UNBlock *UNBlocker = nullptr;
}; // CLASS COMPRESSOR ENDS

/*
 * Class Reader <- Inherits QObject.
 * ===============
 *
 *  Takes care of reading of archives with the help
 *  of libarchive.
 *
 *  Constructors:
 *  ============
 *  	Reader(QObject *parent = NULL) - Constructs an empty Reader Object.
 *  	Reader(const QString &Archive) - Constructs and sets Archive(1).
 *
 *  Methods:
 *  =======
 *      setArchive(const QString &Archive) - Sets the Archive(1) Path.
 *      setPassword(const QString &Password) - Sets the password for the Archive(1).
 *      setAskPassword(bool AskPassword) - If set true , If password is required it will be asked until the user sets the
 *                                         correct password or a empty password.
 *      setBlocksize(int size) - Set the blocksize.
 *      getFilesList(void) - Returns the list of files in QJsonObject.
 *      clear(void) - Clears the reader objects cache.
 *
 *  Slots:
 *  =====
 *      waitForFinished(void) - Blocks the caller thread until the reading is finished or canceled.
 *      start(void) - Starts the reading process in a different thread.
 *      pause(void) - Pauses the reading process.
 *      resume(void) - Resumes the reading process.
 *      cancel(void) - Cancel the reading process.
 *
 *      isRunning() - Returns True if the reading process is running.
 *      isCanceled() - Returns True if the reading process has been canceled.
 *      isPaused() - Returns True if the reading process is paused.
 *      isStarted() - Returns True if the reading process is started.
 *
 *      setFunc(short signal , std::function<void(void)> function) - Connects the lambda function to the Signal with
 *                                                                    respect to the given signal(1) code.
 *                                                                    This slot can connect the following signals.
 *                                                                    started,finished,canceled,paused, and resumed.
 *
 *      setFunc(short signal , std::function<void(QStringJsonObject)> function) - Connects the lambda function to
 *                                                                                filesList signal.
 *      setFunc(std::function<void(int)> function) - Connects the lambda function to password required signal.
 *      setFunc(std::function<void(short,QString)> function) - Connects the lambda function to the error signal.
 *
 *  Signals:
 *  =======
 *      started(void) - Emitted when the reading process is started.
 *      finished(void) - Emitted when the reading process is finished without any error.
 *      canceled(void) - Emitted when the reading process is canceled.
 *      paused(void) - Emitted when the reading process is paused successfully.
 *      resumed(void) - Emitted when the reading process is resumed successfully.
 *      progress(int) - Emitted on progress update.
 *      passwordRequired(int) - Emitted when password is required for reading the archive.
 *      filesList(QJsonObject) - Emitted when reading process is finished.
 *      error(short errorCode , QString eMsg) - Emitted when something goes wrong.
*/
class Reader : public QObject // CLASS READER
{
    Q_OBJECT
public:
    explicit Reader(QObject *parent = nullptr);
    explicit Reader(const QString&);
    Reader &setArchive(const QString&);
#if ARCHIVE_VERSION_NUMBER >= 3003002
    Reader &setPassword(const QString&);
    Reader &setAskPassword(bool);
#endif
    Reader &setBlocksize(int);
    QJsonObject getFilesList(void);
    Reader &clear();
    ~Reader();

public Q_SLOTS:
    Reader &waitForFinished(void);
    Reader &start(void);
    Reader &pause(void);
    Reader &resume(void);
    Reader &cancel(void);

    bool isRunning() const;
    bool isCanceled() const;
    bool isPaused() const;
    bool isStarted() const;

    Reader &setFunc(short, std::function<void(void)>);
#if ARCHIVE_VERSION_NUMBER >= 3003002
    Reader &setFunc(std::function<void(int)>);
#endif
    Reader &setFunc(std::function<void(QJsonObject)>);
    Reader &setFunc(std::function<void(short,QString)>);
private Q_SLOTS:
    int init(void);
    int condition(void);
    int loopContent(void);
    void deinit(int);
    QString getDirectoryFileName(const QString&);
Q_SIGNALS:
    void started();
    void finished();
    void paused();
    void resumed();
    void canceled();
#if ARCHIVE_VERSION_NUMBER >= 3003002
    void passwordRequired(int);
    void submitPassword();
#endif
    void filesList(QJsonObject);
    void error(short, const QString&);

private:
    int ret = 0;
    QSharedPointer<struct archive> archive;
    struct archive_entry *entry;

    QMutex mutex;
#if ARCHIVE_VERSION_NUMBER >= 3003002
    bool AskPassword = false;
    int PasswordTries = 0;
#endif
    int BlockSize = 10240;
    QString ArchivePath;
#if ARCHIVE_VERSION_NUMBER >= 3003002
    QString Password;
#endif
    QJsonObject ArchiveContents;
    UNBlock *UNBlocker = nullptr;

#if ARCHIVE_VERSION_NUMBER >= 3003002
    static const char *password_callback(struct archive *a, void *_client_data)
    {

        (void)a; /* UNUSED */
        Reader *e = (Reader*)_client_data;
        if(e->AskPassword) {
            if(e->PasswordTries > 0 || e->Password.isEmpty()) {
                QEventLoop Freeze;
                e->connect(e, SIGNAL(submitPassword()), &Freeze, SLOT(quit()));
                QTimer::singleShot(1000, [e]() {
                    if(e->PasswordTries > 0) {
                        e->ret = ARCHIVE_WRONG_PASSWORD;
                        e->error(ARCHIVE_WRONG_PASSWORD, e->ArchivePath);
                    }
                    emit(e->passwordRequired(e->PasswordTries));
                }); // emit signal.
                Freeze.exec();
                e->disconnect(e, SIGNAL(submitPassword()), &Freeze, SLOT(quit()));
                if(e->Password.isEmpty()) {
                    e->ret = ARCHIVE_PASSWORD_NOT_GIVEN;
                    e->error(ARCHIVE_PASSWORD_NOT_GIVEN, e->ArchivePath);
                    return NULL;
                }
            }
        } else {
            if(e->Password.isEmpty()) {
                e->ret = ARCHIVE_PASSWORD_NOT_GIVEN;
                e->error(ARCHIVE_PASSWORD_NOT_GIVEN, e->ArchivePath);
                return NULL;
            } else if(e->PasswordTries > 0) {
                e->ret = ARCHIVE_WRONG_PASSWORD;
                e->error(ARCHIVE_WRONG_PASSWORD, e->ArchivePath);
                return NULL;
            }
        }
        e->PasswordTries += 1;
        return e->Password.toUtf8().constData();
    }
// ---
#endif
}; // CLASS READER ENDS
} // QARCHIVE NAMESPACE ENDS
#endif // QARCHIVE_HPP_INCLUDED
