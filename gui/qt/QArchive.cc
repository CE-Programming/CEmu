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
 * -----------------------------------------------------------------------------
 *  @filename 	 	: QArchive.cc
 *  @description 	: Source for QArchive Namespace.
 * -----------------------------------------------------------------------------
*/
#include "QArchive.hpp"

/*
 * Destructors for struct archive ,
 * Which is Handled by smart pointers.
 * ====================================
*/
static void deleteArchiveReader(struct archive *ar)
{
    if(ar) {
        archive_read_close(ar);
        archive_read_free(ar);
    }
    return;
}

static void deleteArchiveWriter(struct archive *aw)
{
    if(aw) {
        archive_write_close(aw);
        archive_write_free(aw);
    }
    return;
}

static void deleteArchiveEntry(struct archive_entry *entry)
{
    if(entry) {
        archive_entry_free(entry);
    }
    return;
}

// ======

using namespace QArchive;

/*
 * UNBlock Class Source.
 *
 *  This class provides a for loop in a
 *  async way. Helps all other classes
 *  for making it async.
 *
 * ============================
 *  Constructor and Destructor.
 * ============================
*/

UNBlock::UNBlock(QObject *parent)
    : QObject(parent)
{
    return;
}

UNBlock::UNBlock(
    std::function<int(void)> initializer,
    std::function<int(void)> condition,
    std::function<void(void)> expression,
    std::function<int(void)> block,
    std::function<void(int)> deinitializer,
    int endpoint,
    int TIterations
)
    : QObject(nullptr)
{
    setInitializer(initializer)
    .setCondition(condition)
    .setExpression(expression)
    .setCodeBlock(block)
    .setDeInitializer(deinitializer)
    .setEndpoint(endpoint)
    .setTotalIterations(TIterations);
    return;
}

UNBlock::~UNBlock()
{
    return;
}

// ===================


/*
 * UNBlock Public Methods.
 * ========================
*/

UNBlock &UNBlock::setInitializer(std::function<int(void)> initialize)
{
    QMutexLocker locker(&Mutex);
    initializer = initialize;
    return *this;
}

UNBlock &UNBlock::setCondition(std::function<int(void)> con)
{
    QMutexLocker locker(&Mutex);
    condition = con;
    return *this;
}

UNBlock &UNBlock::setExpression(std::function<void(void)> exp)
{
    QMutexLocker locker(&Mutex);
    expression = exp;
    return *this;
}

UNBlock &UNBlock::setCodeBlock(std::function<int(void)> block)
{
    QMutexLocker locker(&Mutex);
    codeBlock = block;
    return *this;
}

UNBlock &UNBlock::setDeInitializer(std::function<void(int)> deinit)
{
    QMutexLocker locker(&Mutex);
    deinitializer = deinit;
    return *this;
}


UNBlock &UNBlock::setEndpoint(int epoint)
{
    QMutexLocker locker(&Mutex);
    endpoint = epoint;
    return *this;
}


UNBlock &UNBlock::setTotalIterations(int total)
{
    QMutexLocker locker(&Mutex);
    TIterations = total;
    return *this;
}

// ===================================

/*
 * UNBlock Public Slots.
 * ===================================
*/


UNBlock &UNBlock::waitForFinished(void)
{
    if(!isRunning() || Future == nullptr) {
        return *this;
    }
    Future->waitForFinished(); // Sync.
    return *this;
}

UNBlock &UNBlock::start(void)
{
    QMutexLocker locker(&Mutex);
    if(isRunning() || isPaused() || _bIsCancelRequested) {
        return *this;
    }

    _bStarted = true;

    Future = new QFuture<void>;
    *Future = QtConcurrent::run(this, &UNBlock::loop);
    return *this;
}

UNBlock &UNBlock::cancel(void)
{
    if(!isRunning()) {
        return *this;
    }
    setCancelRequested(true);
    return *this;
}

UNBlock &UNBlock::pause(void)
{
    if(!isStarted()) {
        return *this;
    }
    setPauseRequested(true);
    return *this;
}

UNBlock &UNBlock::resume(void)
{
    if(!isPaused()) {
        return *this;
    }
    QMutexLocker locker(&Mutex);
    emit(doResume());
    return *this;
}

bool UNBlock::isRunning(void) const
{
    bool _bFutureRunning = false;
    if(Future != nullptr) {
        _bFutureRunning = Future->isRunning();
    }
    return (_bStarted | _bFutureRunning);
}

bool UNBlock::isCanceled(void) const
{
    return _bCanceled;
}

bool UNBlock::isPaused(void) const
{
    return _bPaused;
}

bool UNBlock::isStarted(void) const
{
    return _bStarted;
}

// =======================================

/*
 * UNBlock Private Slots.
 * =======================================
*/

void UNBlock::loop(void)
{
    if(initializer) {
        if(initializer()) { // This has to be done once.
            setStarted(false);
            setCanceled(true);
            if(deinitializer) {
                deinitializer(1); // error.
            }
            emit(canceled());
            return;
        }
    }
    emit(started());

    if(!condition) {
        if(deinitializer) {
            deinitializer(1); // error.
        }
        // reset
        {
            QMutexLocker locker(&Mutex);
            _bStarted = _bPaused = _bCanceled = _bIsCancelRequested = _bIsPauseRequested = false;
        }
        // ---
        emit(finished());
        return;
    }

    int counter = 1;
    while(condition() != endpoint) {
        // Check if cancel or pause is shot.
        {
            if(_bIsCancelRequested) {
                setCancelRequested(false);
                setCanceled(true);
                if(deinitializer) {
                    deinitializer(1); // report error.
                }
                emit(canceled());
                return;
            } else if(_bIsPauseRequested) {
                QEventLoop Freeze;
                setPauseRequested(false);
                setPaused(true);

                connect(this, SIGNAL(doResume()), &Freeze, SLOT(quit()));
                emit(paused());

                Freeze.exec(); // Freezes the thread.
                /*
                 * This section will
                 * be executed when resume
                 * slot is shot.
                */
                setStarted(true);
                setPaused(false);
                disconnect(this, SIGNAL(doResume()), &Freeze, SLOT(quit()));
                emit(resumed());
                // ------
            }
        }
        // ------

        /*
         * Execute Instructions in
         * the loop.
        */
        if(codeBlock) {
            if(codeBlock()) {
                setStarted(false);
                setCanceled(true);
                if(deinitializer) {
                    deinitializer(1); // Non-Zero -> error.
                }
                emit(canceled());
                return;
            }
        }
        // ------

        /*
         * Give Progress for the
         * user if available.
        */
        if(TIterations) { // Avoid zero division error.
            int percentage = static_cast<int>(((counter) * 100.0) / (TIterations));
            emit(progress(percentage));
            ++counter;
        }
        // ------

        /*
         * Expression /
         * Increament.
        */
        if(expression) {
            expression();
        }
        // ------
    }

    // deinit
    if(deinitializer) {
        deinitializer(0); // Zero -> no error.
    }

    // reset
    {
        QMutexLocker locker(&Mutex);
        _bStarted = _bPaused = _bCanceled = _bIsCancelRequested = _bIsPauseRequested = false;
    }
    // ---

    emit(finished());
    return;
}

void UNBlock::setStarted(bool ch)
{
    QMutexLocker locker(&Mutex);
    _bStarted = ch;
    return;
}
void UNBlock::setPaused(bool ch)
{
    QMutexLocker locker(&Mutex);
    _bPaused = ch;
    return;
}

void UNBlock::setCanceled(bool ch)
{
    QMutexLocker locker(&Mutex);
    _bCanceled = ch;
    return;
}

void UNBlock::setCancelRequested(bool ch)
{
    QMutexLocker locker(&Mutex);
    _bIsCancelRequested = ch;
    return;
}

void UNBlock::setPauseRequested(bool ch)
{
    QMutexLocker locker(&Mutex);
    _bIsPauseRequested = ch;
    return;
}

// =======================


/*
 * Extractor Class Source.
 * ============================
 * Constructor and Destructor.
 * ============================
*/
Extractor::Extractor(QObject *parent)
    : QObject(parent)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    return;
}

Extractor::Extractor(const QString &filename)
    : QObject(nullptr)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    setArchive(filename);
    return;
}

Extractor::Extractor(const QString &filename, const QString &destination)
    : QObject(nullptr)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    setArchive(filename, destination);
    return;
}

Extractor::~Extractor()
{
    if(isPaused()) {
        resume();
    }
    if(isRunning()) {
        cancel();
        waitForFinished();
        UNBlocker->disconnect();
        // blocks any-thread until every
        // job is stopped successfully ,
        // Otherwise this can cause a
        // segfault.
    }
    return;
}

/* ============================================== */

/*
 * Public Methods.
 * ==============================================
 *  Extractor
*/
Extractor &Extractor::setArchive(const QString &filename)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    clear();
    ArchivePath = QString(filename);
    return *this;
}

Extractor &Extractor::setArchive(const QString &filename, const QString &destination)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    clear();
    ArchivePath = QString(filename);
    Destination = cleanDestPath(destination);
    return *this;
}

#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
Extractor &Extractor::setPassword(const QString &pwd)
{
    QMutexLocker locker(&mutex);
    Password = QString(pwd);
    emit(submitPassword());
    return *this;
}

Extractor &Extractor::setAskPassword(bool ch)
{
    QMutexLocker locker(&mutex);
    AskPassword = ch;
    emit(submitPassword()); // Just in case.
    return *this;
}
#endif

Extractor &Extractor::setBlocksize(int size)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    BlockSize = size;
    return *this;
}

Extractor &Extractor::onlyExtract(const QString &filepath)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    if(OnlyExtract.contains(filepath)) {
        return *this;
    }
    OnlyExtract << (Destination + filepath);
    return *this;
}

Extractor &Extractor::onlyExtract(const QStringList &filepaths)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    Q_FOREACH(QString filepath, filepaths) {
        if(!OnlyExtract.contains(filepath)) {
            OnlyExtract << (Destination + filepath);
        }
    }
    return *this;
}

Extractor &Extractor::clear(void)
{
    if(isRunning() || isPaused()) {
        return *this;
    }

    archive.reset();
    ext.reset();

    ret = 0;
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    PasswordTries = 0;
    AskPassword = false;
#endif
    BlockSize = 10240;
    ArchivePath.clear();
    Destination.clear();
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    Password.clear();
#endif
    OnlyExtract.clear();
    return *this;
}

/* ==================================== */

/*
 * Public Slots.
 * ======================================================================
 *  Extractor
*/
Extractor &Extractor::waitForFinished(void)
{
    UNBlocker->waitForFinished(); // Sync.
    return *this;
}

Extractor &Extractor::start(void)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isStarted() || isPaused()) {
        return *this;
    }

    UNBlocker->setInitializer([this]() -> int { return init(); })
    .setCondition([this]() -> int { return condition(); })
    .setEndpoint(ARCHIVE_EOF)
    .setCodeBlock([this]() {
        return loopContent();
    })
    .start();

    return *this;
}

Extractor &Extractor::pause(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->pause();
    return *this;
}

Extractor &Extractor::resume(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->resume();
    return *this;
}

Extractor &Extractor::cancel(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->cancel();
    return *this;
}

bool Extractor::isRunning() const
{
    return UNBlocker->isRunning();
}
bool Extractor::isCanceled() const
{
    return UNBlocker->isCanceled();
}
bool Extractor::isPaused() const
{
    return UNBlocker->isPaused();
}
bool Extractor::isStarted() const
{
    return UNBlocker->isStarted();
}

Extractor &Extractor::setFunc(short signal, std::function<void(void)> function)
{
    QMutexLocker locker(&mutex);
    switch(signal) {
    case STARTED:
        connect(this, &Extractor::started, function);
        break;
    case FINISHED:
        connect(this, &Extractor::finished, function);
        break;
    case PAUSED:
        connect(this, &Extractor::paused, function);
        break;
    case RESUMED:
        connect(this, &Extractor::resumed, function);
        break;
    case CANCELED:
        connect(this, &Extractor::canceled, function);
        break;
    default:
        break;
    };
    return *this;
}

Extractor &Extractor::setFunc(short signal, std::function<void(QString)> function)
{
    QMutexLocker locker(&mutex);
    if(signal == EXTRACTED) {
        connect(this, &Extractor::extracted, function);
    } else {
        connect(this, &Extractor::extracting, function);
    }
    return *this;
}

Extractor &Extractor::setFunc(std::function<void(short,QString)> function)
{
    QMutexLocker locker(&mutex);
    connect(this, &Extractor::error, function);
    return *this;
}

Extractor &Extractor::setFunc(short signal, std::function<void(int)> function)
{
    QMutexLocker locker(&mutex);
    if(signal == PROGRESS) {
        connect(this, &Extractor::progress, function);
    } else {
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
        connect(this, &Extractor::passwordRequired, function);
#endif
    }
    return *this;
}

/* ====================================================================== */

/*
 * Private Slots.
 * ===============================================================================
 *  Extractor
 *  Internals for the extractor.
*/
int Extractor::init(void)
{
    // Check if the archive and the destination folder exists
    QFileInfo check_archive(ArchivePath);
    QFileInfo check_dest(Destination);
    // check if file exists and if yes then is it really a file and no directory?
    if (!check_archive.exists() || !check_archive.isFile()) {
        error(ARCHIVE_READ_ERROR, ArchivePath);
        return ARCHIVE_READ_ERROR;
    } else {
        if(!Destination.isEmpty()) {
            if(!check_dest.exists() || !check_dest.isDir()) {
                error(DESTINATION_NOT_FOUND, Destination);
                return DESTINATION_NOT_FOUND;
            }
        }
    }
    // ---

    archive = QSharedPointer<struct archive>(archive_read_new(), deleteArchiveReader);
    ext = QSharedPointer<struct archive>(archive_write_disk_new(), deleteArchiveWriter);
    if(!(archive.data() && ext.data())) {
        // No memory.
        error(NOT_ENOUGH_MEMORY, ArchivePath);
        return NOT_ENOUGH_MEMORY;
    }
    archive_write_disk_set_options(ext.data(), flags);
    archive_read_support_format_all(archive.data());
    archive_read_support_filter_all(archive.data());
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    archive_read_set_passphrase_callback(archive.data(), (void*)this, password_callback);
#endif

    if((ret = archive_read_open_filename(archive.data(), QFile::encodeName(ArchivePath).constData(), BlockSize))) {
        error(ARCHIVE_READ_ERROR, ArchivePath);
        return ARCHIVE_READ_ERROR;
    }
    // ---

    int TotalIterations = 0;
    if(OnlyExtract.isEmpty()) {
        TotalIterations = totalFileCount(); // gets the total number of files in the archive.
        if(TotalIterations == -1) {
            /*
             * Since we checked for possible errors earlier and
             * we don't have more info on this error we simply
             * raise a uncaught error.
            */
            error(ARCHIVE_UNCAUGHT_ERROR, ArchivePath);
            return ARCHIVE_UNCAUGHT_ERROR;
        }
    } else {
        TotalIterations = OnlyExtract.size();
    }

    UNBlocker->setTotalIterations(TotalIterations); // This makes it possible for the progress.
    return 0; // return no error.

}

int Extractor::condition(void)
{
    ret = archive_read_next_header(archive.data(), &entry);
    if (ret == ARCHIVE_FATAL) {
        ret = ARCHIVE_EOF; // set to endpoint to stop UNBlock.
        error(ARCHIVE_QUALITY_ERROR, ArchivePath);
    }
    return ret;
}

int Extractor::loopContent(void)
{
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    PasswordTries = 0; // reset
#endif
    if(!Destination.isEmpty()) {
        char* new_entry = concat(Destination.toUtf8().constData(), archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, new_entry);
        free(new_entry);
    }

    QString checkfile = QString(archive_entry_pathname(entry));
    bool extract = (!OnlyExtract.isEmpty() && !OnlyExtract.contains(checkfile)) ? false : true;
    if(extract) {
        // Signal that we are extracting this file.
        emit(extracting(checkfile));
        // ---

        ret = archive_write_header(ext.data(), entry);
        if (ret == ARCHIVE_OK) {
            {
                const void *buff;
                size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
                int64_t offset;
#else
                off_t offset;
#endif
                for (;;) {
                    ret = archive_read_data_block(archive.data(), &buff, &size, &offset);
                    if (ret == ARCHIVE_EOF) {
                        break;
                    } else if (ret != ARCHIVE_OK) {
                        ret = ARCHIVE_EOF;
                        error(ARCHIVE_UNCAUGHT_ERROR, checkfile);
                        return ARCHIVE_UNCAUGHT_ERROR;
                    } else {
                        ret = archive_write_data_block(ext.data(), buff, size, offset);
                        if (ret != ARCHIVE_OK) {
                            ret = ARCHIVE_EOF;
                            error(ARCHIVE_UNCAUGHT_ERROR, checkfile);
                            return ARCHIVE_UNCAUGHT_ERROR;
                        }
                    }
                }
            }
            ret = archive_write_finish_entry(ext.data());
            if (ret == ARCHIVE_FATAL) {
                ret = ARCHIVE_EOF;
                error(ARCHIVE_UNCAUGHT_ERROR, checkfile);
                return ARCHIVE_UNCAUGHT_ERROR;
            }
            ret = ARCHIVE_OK;

            // Signal that we successfully extracted this file.
            emit(extracted(checkfile));
            // ---
        }

    }
    return 0; // return no error

}


int Extractor::totalFileCount(void)
{
    auto a = QSharedPointer<struct archive>(archive_read_new(), deleteArchiveReader);
    struct archive_entry *e;
    if(!(a.data())) {
        return -1;
    }
    archive_read_support_format_all(a.data());
    archive_read_support_filter_all(a.data());
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    archive_read_add_passphrase(a.data(), Password.toUtf8().constData());
#endif
    if((archive_read_open_filename(a.data(), ArchivePath.toUtf8().constData(), BlockSize))) {
        return -1;
    }
    while(ARCHIVE_OK == archive_read_next_header(a.data(), &e)) {
        continue;
    }
    return (archive_errno(a.data())) ? -1 : archive_file_count(a.data());
}

QString Extractor::cleanDestPath(const QString& input)
{
    QString ret = QDir::cleanPath(QDir::toNativeSeparators(input));
    if(ret.at(ret.count() - 1) != QDir::separator()) {
        ret += QDir::separator();
    }
    return ret;
}

char *Extractor::concat(const char *dest, const char *src)
{
    char *ret = (char*) calloc(sizeof(char), strlen(dest) + strlen(src) + 1);

    strcpy(ret, dest);
    strcat(ret, src);

    return ret;
}


/*
 * Class Compressor
 * ====================================================
 *  QArchive Compressor Source
 * ====================================================
 * Constructor and Destructor.
 * ====================================================
*/
Compressor::Compressor(QObject *parent)
    : QObject(parent)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    return;
}

Compressor::Compressor(const QString& archive)
    : QObject(nullptr)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    setArchive(archive);
    return;
}

Compressor::Compressor(const QString& archive, const QStringList& files)
    : QObject(nullptr)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    setArchive(archive);
    addFiles(files);
    return;

}

Compressor::Compressor(const QString& archive, const QString& file)
    : QObject(nullptr)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    connect(UNBlocker, SIGNAL(progress(int)), this, SIGNAL(progress(int)));
    setArchive(archive);;
    addFiles(file);
    return;
}

Compressor::~Compressor()
{
    if(isPaused()) {
        resume();
    }
    if(isRunning()) {
        cancel();
        waitForFinished();
        UNBlocker->disconnect();
        // blocks any-thread until every
        // job is stopped successfully ,
        // Otherwise this can cause a
        // segfault.
    }
    return;
}

/* ================================================================== */



/*
 * Public Methods.
 * =======================================================================
 *  Compressor.
*/
Compressor &Compressor::setArchive(const QString &archive)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    archivePath = QDir::cleanPath(archive);
    return *this;
}

Compressor &Compressor::setArchive(const QString &archive, const QString &file)
{
    setArchive(archive);
    addFiles(file);
    return *this;
}

Compressor &Compressor::setArchive(const QString &archive, const QStringList &files)
{
    setArchive(archive);
    addFiles(files);
    return *this;
}

Compressor &Compressor::setArchiveFormat(short type)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    archiveFormat = type;
    return *this;
}

#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
Compressor &Compressor::setPassword(const QString &pwd)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    Password = QString(pwd);
    return *this;
}
#endif

Compressor &Compressor::setCompressionLevel(int level)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    CompressionLevel = level;
    return *this;
}

Compressor &Compressor::setBlocksize(int size)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    BlockSize = size;
    return *this;
}

Compressor &Compressor::addFiles(const QString& file)
{
    /*
     * No like files can exist in a filesystem!
    */
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    if(!nodes.contains(file)) {
        QFileInfo checkfile(file);
        if(checkfile.isFile()) {
            nodes.insert( file, checkfile.fileName());
        } else {
            nodes.insert( file, file );  // Later will be populated.
        }
    }
    return *this;
}

Compressor &Compressor::addFiles(const QStringList& files)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    Q_FOREACH(QString file, files) {
        if(!nodes.contains(file)) {
            QFileInfo checkfile(file);
            if(checkfile.isFile()) {
                nodes.insert( file, checkfile.fileName());
            } else {
                nodes.insert( file, file ); // This will be later populated.
            }
        }
    }
    return *this;
}

Compressor &Compressor::removeFiles(const QString& file)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    nodes.remove(file);
    return *this;
}

Compressor &Compressor::removeFiles(const QStringList& files)
{
    QMutexLocker locker(&mutex);
    if(nodes.isEmpty() || isRunning() || isPaused()) {
        return *this;
    }
    Q_FOREACH(QString file, files) {
        nodes.remove(file);
    }
    return *this;
}

Compressor &Compressor::clear(void)
{
    if(isRunning() || isPaused()) {
        return *this;
    }
    CompressionLevel = 0; // Use Default.
    archiveFormat = NO_FORMAT; // Defaults to tar.
    archivePath.clear();
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    Password.clear();
#endif
    nodes.clear();
    tempFile.reset();
    archive.reset();
    return *this;
}

/* =================================================== */


/*
 * Public Slots.
 * ========================================================================
 *  Compressor
*/
Compressor &Compressor::waitForFinished(void)
{
    /*
     * Note: Do not use mutex here.
    */
    UNBlocker->waitForFinished(); // Sync.
    return *this;
}

Compressor &Compressor::start(void)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isStarted() || isPaused()) {
        return *this;
    }

    UNBlocker->setInitializer([this]() -> int { return init(); })
    .setCondition([this]() -> int { return condition(); })
    .setEndpoint(1)
    .setExpression([this]() {
        expression();
    })
    .setCodeBlock([this]() {
        return loopContent();
    })
    .setDeInitializer([this](int cancel) {
        deinit(cancel);
    })
    .setTotalIterations(nodes.size())
    .start();

    return *this;
}

Compressor &Compressor::pause(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->pause();
    return *this;
}

Compressor &Compressor::resume(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->resume();
    return *this;
}

Compressor &Compressor::cancel(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->cancel();
    return *this;
}

bool Compressor::isRunning() const
{
    return UNBlocker->isRunning();
}
bool Compressor::isCanceled() const
{
    return UNBlocker->isCanceled();
}
bool Compressor::isPaused() const
{
    return UNBlocker->isPaused();
}
bool Compressor::isStarted() const
{
    return UNBlocker->isStarted();
}

Compressor &Compressor::setFunc(short signal, std::function<void(void)> function)
{
    QMutexLocker locker(&mutex);
    switch(signal) {
    case STARTED:
        connect(this, &Compressor::started, function);
        break;
    case FINISHED:
        connect(this, &Compressor::finished, function);
        break;
    case PAUSED:
        connect(this, &Compressor::paused, function);
        break;
    case RESUMED:
        connect(this, &Compressor::resumed, function);
        break;
    case CANCELED:
        connect(this, &Compressor::canceled, function);
        break;
    default:
        break;
    };
    return *this;
}

Compressor &Compressor::setFunc(short signal, std::function<void(QString)> function)
{
    QMutexLocker locker(&mutex);
    if(signal == COMPRESSED) {
        connect(this, &Compressor::compressed, function);
    } else {
        connect(this, &Compressor::compressing, function);
    }
    return *this;
}

Compressor &Compressor::setFunc(std::function<void(short,QString)> function)
{
    QMutexLocker locker(&mutex);
    connect(this, &Compressor::error, function);
    return *this;
}

Compressor &Compressor::setFunc(std::function<void(int)> function)
{
    QMutexLocker locker(&mutex);
    connect(this, &Compressor::progress, function);
    return *this;
}

/* =========================================== */

/*
 * Private Slots.
 * ==============================================================================
 *  Compressor
*/

int Compressor::init(void)
{
    checkNodes(); // clear unwanted files
    populateDirectory(); // fills in the directories.
    if(nodes.isEmpty() || archivePath.isEmpty()) {
        return 1; // exits the operation.
    }
    if(archiveFormat == NO_FORMAT) {
        getArchiveFormat();
    }
    bool noTar = false;

    archive =  QSharedPointer<struct archive>(archive_write_new(), deleteArchiveWriter);

    switch (archiveFormat) {
    case BZIP:
    case BZIP2:
        archive_write_add_filter_bzip2(archive.data());
        break;
    case GZIP:
        archive_write_add_filter_gzip(archive.data());
        break;
    case NO_FORMAT:
        noTar = false;
        break;
    default:
        noTar = true;
        archive_write_add_filter_none(archive.data());
        break;
    }
    if(noTar) {
        if(archiveFormat == SEVEN_ZIP) {
            archive_write_set_format_7zip(archive.data());
        }
        if(archiveFormat == XAR) {
            archive_write_set_format_xar(archive.data());
        }
        if(archiveFormat == ZIP) {
            archive_write_set_format_zip(archive.data());
        }
    } else {
        archive_write_set_format_ustar(archive.data());
    }

    tempFile = QSharedPointer<QSaveFile>(new QSaveFile(archivePath));
    if(!tempFile->open(QIODevice::WriteOnly)) {
        emit(error(ARCHIVE_WRITE_OPEN_ERROR, archivePath));
        return ARCHIVE_WRITE_OPEN_ERROR;
    }

    /*
     * Set config for the new archive.
     * ================================
    */
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    /*
     * Currently I don't know if libarchive supports passwords that well.
     * So for the time beign lets keep this empty.
    */
    if(!Password.isEmpty() && (archiveFormat == ZIP || archiveFormat == SEVEN_ZIP)) {
        archive_write_set_passphrase(archive.data(), Password.toUtf8().constData());
    }
#endif

    if(CompressionLevel) {
        QByteArray options("compression-level=");
        options += QByteArray::number(CompressionLevel);
        archive_write_set_options(archive.data(), options.constData());
    }
    archive_write_set_bytes_per_block(archive.data(), BlockSize);
    // =======================

    archive_write_open_fd(archive.data(), tempFile->handle()); // Open.
    // Start the Map Iterator.
    mapIter = nodes.begin();
    // ====

    return 0; // no error.
}

int Compressor::condition(void)
{
    return (mapIter == nodes.end());
}

void Compressor::expression(void)
{
    ++mapIter;
    return;
}

int Compressor::loopContent(void)
{
    int r;
    ssize_t len;
    char buff[16384];

    // Signal that we are compressing this file.
    emit(compressing(mapIter.key()));

    auto disk = QSharedPointer<struct archive>(archive_read_disk_new(), deleteArchiveReader);
    archive_read_disk_set_standard_lookup(disk.data());

    r = archive_read_disk_open(disk.data(), QFile::encodeName(mapIter.key()).constData());
    if (r != ARCHIVE_OK) {
        emit error(DISK_OPEN_ERROR, mapIter.key());
        return DISK_OPEN_ERROR;
    }

    for (;;) {
        auto entry = QSharedPointer<struct archive_entry>(archive_entry_new(), deleteArchiveEntry);
        r = archive_read_next_header2(disk.data(), entry.data());
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r != ARCHIVE_OK) {
            emit error(DISK_READ_ERROR, mapIter.key());
            return DISK_READ_ERROR;
        }
        archive_read_disk_descend(disk.data());
        archive_entry_set_pathname(entry.data(), mapIter.value().toUtf8().constData());
        r = archive_write_header(archive.data(), entry.data());

        if (r == ARCHIVE_FATAL) {
            emit error(ARCHIVE_FATAL_ERROR, mapIter.value());
            return ARCHIVE_FATAL_ERROR;
        }
        if (r > ARCHIVE_FAILED) {
            QFile file(mapIter.key());
            if(!file.open(QIODevice::ReadOnly)) {
                emit error(DISK_OPEN_ERROR, mapIter.key());
                return DISK_OPEN_ERROR;
            }
            len = file.read(buff, sizeof(buff));
            while (len > 0) {
                archive_write_data(archive.data(), buff, len);
                len = file.read(buff, sizeof(buff));
            }
            // Close the read
            file.close();
        }
    }

    // Signal that we compressed this file.
    emit compressed(mapIter.key());

    return 0;
}

void Compressor::deinit(int canceled)
{
    CompressionLevel = 0;
    archiveFormat = NO_FORMAT;
    archivePath.clear();
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    Password.clear();
#endif
    nodes.clear();
    archive.reset();
    if(canceled) {
        tempFile.reset();
    } else {
        tempFile->commit(); // Write the archive.
        tempFile.reset();
    }
    return;
}

QString Compressor::isDirInFilesList(void)
{
    QString ret;
    for (auto iter = nodes.begin() ; iter != nodes.end() ; ++iter) {
        if(QFileInfo(iter.key()).isDir()) {
            ret = QString(iter.key());
            nodes.erase(iter); // remove it from the map
            break;
        }
    }
    return ret;
}

void Compressor::populateDirectory(void)
{
    QString entry;
    while(!(entry = isDirInFilesList()).isEmpty()) {
        qDebug() << "Entry:: " << entry;
        QDir dir(entry);
        QFileInfoList list = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
        for (int i = 0; i < list.size(); i++) {
            QString file = list.at(i).filePath();
            nodes.insert( file, file );
        }
        entry.clear();
    }
    return;
}

void Compressor::checkNodes(void)
{
    Q_FOREACH(QString key, nodes.uniqueKeys()) {
        QFileInfo fInfo(key);
        if(!fInfo.exists()) {
            nodes.remove(key);
            emit error(FILE_NOT_EXIST, key);
        }
    }
    return;
}

void Compressor::getArchiveFormat()
{
    QFileInfo fInfo(archivePath);
    QString ext = fInfo.suffix();

    if(ext.toLower() == "bz") {
        archiveFormat = BZIP;
    } else if(ext.toLower() == "bz2") {
        archiveFormat = BZIP2;
    } else if(ext.toLower() == "gz") {
        archiveFormat = GZIP;
    } else if(ext.toLower() == "cpio") {
        archiveFormat = CPIO;
    } else if(ext.toLower() == "xar") {
        archiveFormat = XAR;
    } else if(ext.toLower() == "zip") {
        archiveFormat = ZIP;
    } else if(ext.toLower() == "7z") {
        archiveFormat = SEVEN_ZIP;
    } else {
        archiveFormat = NO_FORMAT; // default
    }
}

/*
 * Reader Class Source.
 * ======================================
 * Constructor and Destructor.
 * ======================================
*/
Reader::Reader(QObject *parent)
    : QObject(parent)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    return;
}

Reader::Reader(const QString& archive)
    : QObject(nullptr)
{
    UNBlocker = new UNBlock(this);
    connect(UNBlocker, SIGNAL(started()), this, SIGNAL(started()));
    connect(UNBlocker, SIGNAL(paused()), this, SIGNAL(paused()));
    connect(UNBlocker, SIGNAL(resumed()), this,SIGNAL(resumed()));
    connect(UNBlocker, SIGNAL(canceled()), this,SIGNAL(canceled()));
    connect(UNBlocker, SIGNAL(finished()), this, SIGNAL(finished()));
    setArchive(archive);
    return;
}

Reader::~Reader()
{
    if(isPaused()) {
        resume();
    }
    if(isRunning() || isStarted()) {
        cancel();
        waitForFinished();
        UNBlocker->disconnect();
    }
    return;
}
// =================================

/*
 * Public Methods.
 * =====================================================
 *  Reader
*/
Reader &Reader::setArchive(const QString &filename)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    clear();
    ArchivePath = QString(filename);
    return *this;
}

#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
Reader &Reader::setPassword(const QString &pwd)
{
    QMutexLocker locker(&mutex);
    Password = QString(pwd);
    emit(submitPassword());
    return *this;
}

Reader &Reader::setAskPassword(bool ch)
{
    QMutexLocker locker(&mutex);
    AskPassword = ch;
    emit(submitPassword()); // Just in case.
    return *this;
}
#endif

Reader &Reader::setBlocksize(int size)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isPaused()) {
        return *this;
    }
    BlockSize = size;
    return *this;
}

QJsonObject Reader::getFilesList(void)
{
    QMutexLocker locker(&mutex);
    return ArchiveContents;
}

Reader &Reader::clear(void)
{
    if(isRunning() || isPaused()) {
        return *this;
    }

    archive.reset();
    ret = 0;
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    PasswordTries = 0;
    AskPassword = false;
#endif
    BlockSize = 10240;
    ArchivePath.clear();
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    Password.clear();
#endif
    ArchiveContents = QJsonObject();
    return *this;
}

/* =========================================== */

/*
 * Public Slots.
 * =================================================================
 *  Reader
*/
Reader &Reader::waitForFinished(void)
{
    UNBlocker->waitForFinished(); // Sync.
    return *this;
}

Reader &Reader::start(void)
{
    QMutexLocker locker(&mutex);
    if(isRunning() || isStarted() || isPaused()) {
        return *this;
    }

    UNBlocker->setInitializer([this]() -> int { return init(); })
    .setCondition([this]() -> int { return condition(); })
    .setEndpoint(ARCHIVE_EOF)
    .setCodeBlock([this]() {
        return loopContent();
    })
    .setDeInitializer([this](int canceled) {
        deinit(canceled);
    })
    .start();

    return *this;
}

Reader &Reader::pause(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->pause();
    return *this;
}

Reader &Reader::resume(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->resume();
    return *this;
}

Reader &Reader::cancel(void)
{
    QMutexLocker locker(&mutex);
    UNBlocker->cancel();
    return *this;
}

bool Reader::isRunning() const
{
    return UNBlocker->isRunning();
}
bool Reader::isCanceled() const
{
    return UNBlocker->isCanceled();
}
bool Reader::isPaused() const
{
    return UNBlocker->isPaused();
}
bool Reader::isStarted() const
{
    return UNBlocker->isStarted();
}

Reader &Reader::setFunc(short signal, std::function<void(void)> function)
{
    QMutexLocker locker(&mutex);
    switch(signal) {
    case STARTED:
        connect(this, &Reader::started, function);
        break;
    case FINISHED:
        connect(this, &Reader::finished, function);
        break;
    case PAUSED:
        connect(this, &Reader::paused, function);
        break;
    case RESUMED:
        connect(this, &Reader::resumed, function);
        break;
    case CANCELED:
        connect(this, &Reader::canceled, function);
        break;
    default:
        break;
    };
    return *this;
}

#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
Reader &Reader::setFunc(std::function<void(int)> function)
{
    QMutexLocker locker(&mutex);
    connect(this, &Reader::passwordRequired, function);
    return *this;
}
#endif

Reader &Reader::setFunc(std::function<void(QJsonObject)> function)
{
    QMutexLocker locker(&mutex);
    connect(this, &Reader::filesList, function);
    return *this;
}

Reader &Reader::setFunc(std::function<void(short,QString)> function)
{
    QMutexLocker locker(&mutex);
    connect(this, &Reader::error, function);
    return *this;
}

/* ================================== */

/*
 * Private Slots.
 * =========================================================
 *  Reader
 *  Internals for the reader.
*/
int Reader::init(void)
{
    // Check if the archive exists
    QFileInfo check_archive(ArchivePath);
    // check if file exists and if yes then is it really a file and no directory?
    if (!check_archive.exists() || !check_archive.isFile()) {
        error(ARCHIVE_READ_ERROR, ArchivePath);
        return ARCHIVE_READ_ERROR;
    }
    // ---

    archive = QSharedPointer<struct archive>(archive_read_new(), deleteArchiveReader);
    if(!(archive.data())) {
        // No memory.
        error(NOT_ENOUGH_MEMORY, ArchivePath);
        return NOT_ENOUGH_MEMORY;
    }
    archive_read_support_format_all(archive.data());
    archive_read_support_filter_all(archive.data());
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    archive_read_set_passphrase_callback(archive.data(), (void*)this, password_callback);
#endif

    if((ret = archive_read_open_filename(archive.data(), QFile::encodeName(ArchivePath).constData(), BlockSize))) {
        error(ARCHIVE_READ_ERROR, ArchivePath);
        return ARCHIVE_READ_ERROR;
    }
    // ---

    return 0; // return no error.

}

int Reader::condition(void)
{
    ret = archive_read_next_header(archive.data(), &entry);
    if (ret == ARCHIVE_FATAL) {
        ret = ARCHIVE_EOF; // set to endpoint to stop UNBlock.
        error(ARCHIVE_QUALITY_ERROR, ArchivePath);
    }
    return ret;
}

int Reader::loopContent(void)
{
#if ARCHIVE_VERSION_NUMBER >= 3003002 && QARCHIVE_PASSWORD_SUPPORT == 1
    PasswordTries = 0; // reset
#endif
    QString CurrentFile = QString(archive_entry_pathname(entry));
    QJsonObject CurrentEntry;

    auto entry_stat = archive_entry_stat(entry);
    qint64 size = (qint64)entry_stat->st_size;
    QString sizeUnits = "Bytes";

    if(size == 0) {
        sizeUnits = "None";
        size = 0;
    } else if(size < 1024) {
        sizeUnits = "Bytes";
        size = size;
    } else if(size >= 1024 && size < 1048576) {
        sizeUnits = "KiB";
        size /= 1024;
    } else if(size >= 1048576 && size < 1073741824) {
        sizeUnits = "MiB";
        size /= 1048576;
    } else {
        sizeUnits = "GiB";
        size /= 1073741824;
    }

    // MSVC (and maybe Windows in general?) workaround
    #if defined(_WIN32) && !defined(__CYGWIN__)
    qint64 blockSizeInBytes = 512;
    qint64 blocks = (qint64) ceil(entry_stat->st_size / blockSizeInBytes);
    #else
    qint64 blockSizeInBytes = (qint64)entry_stat->st_blksize;
    qint64 blocks = (qint64)entry_stat->st_blocks;
    #endif

    // For portability reasons
    #if __APPLE__
        #define st_atim st_atimespec.tv_sec
        #define st_ctim st_ctimespec.tv_sec
        #define st_mtim st_mtimespec.tv_sec
    #elif defined(_WIN32) && !defined(__CYGWIN__)
        #define st_atim st_atime
        #define st_ctim st_ctime
        #define st_mtim st_mtime
    #else
        #define st_atim st_atim.tv_sec
        #define st_ctim st_ctim.tv_sec
        #define st_mtim st_mtim.tv_sec
    #endif
    auto lastAccessT = entry_stat->st_atim;
    auto lastModT = entry_stat->st_mtim;
    auto lastStatusModT = entry_stat->st_ctim;
    #if __APPLE__ || (defined(_WIN32) && !defined(__CYGWIN__))
        #undef st_atim
        #undef st_ctim
        #undef st_mtim
    #endif

    QFileInfo fileInfo(CurrentFile);

    auto ft = archive_entry_filetype(entry);
    QString FileType;
    switch(ft) {
    case AE_IFREG: // Regular file
        FileType = "RegularFile";
        break;
    case AE_IFLNK: // Link
        FileType = "SymbolicLink";
        break;
    case AE_IFSOCK: // Socket
        FileType = "Socket";
        break;
    case AE_IFCHR: // Character Device
        FileType = "CharacterDevice";
        break;
    case AE_IFBLK: // Block Device
        FileType = "BlockDevice";
        break;
    case AE_IFDIR: // Directory.
        FileType = "Directory";
        break;
    case AE_IFIFO: // Named PIPE. (fifo)
        FileType = "NamedPipe";
        break;
    default:
        FileType = "UnknownFile";
        break;
    };
    // Set the values.
    if(FileType != "RegularFile") {
        CurrentEntry.insert("FileName", getDirectoryFileName(CurrentFile));
    } else {
        CurrentEntry.insert("FileName", fileInfo.fileName());
    }

    CurrentEntry.insert("FileType", QJsonValue(FileType));
    CurrentEntry.insert("Size", QJsonValue(size));
    CurrentEntry.insert("SizeUnit", sizeUnits);
    CurrentEntry.insert("BlockSize", QJsonValue(blockSizeInBytes));
    CurrentEntry.insert("BlockSizeUnit", "Bytes");
    CurrentEntry.insert("Blocks", QJsonValue(blocks));
    if(lastAccessT) {
        CurrentEntry.insert("LastAccessedTime", QJsonValue((QDateTime::fromTime_t(lastAccessT)).toString(Qt::ISODate)));
    } else {
        CurrentEntry.insert("LastAccessedTime", "Unknown");
    }

    if(lastModT) {
        CurrentEntry.insert("LastModifiedTime", QJsonValue((QDateTime::fromTime_t(lastModT)).toString(Qt::ISODate)));
    } else {
        CurrentEntry.insert("LastModifiedTime", "Unknown");
    }

    if(lastStatusModT) {
        CurrentEntry.insert("LastStatusModifiedTime", QJsonValue((QDateTime::fromTime_t(lastStatusModT)).toString(Qt::ISODate)));
    } else {
        CurrentEntry.insert("LastStatusModifiedTime", "Unknown");
    }

    // Join to the main QJsonObject.
    ArchiveContents.insert(CurrentFile, CurrentEntry);
    return 0; // return no error

}

void Reader::deinit(int canceled)
{
    if(!canceled) {
        emit(filesList(ArchiveContents));
    }
    return;
}

QString Reader::getDirectoryFileName(const QString &dir)
{
    if(dir[dir.count() - 1] == QStringLiteral("/")) {
        return dir.mid(0, dir.count() - 1);
    }
    return dir;
}

// ================================
