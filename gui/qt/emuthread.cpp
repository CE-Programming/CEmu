#include "emuthread.h"

#include <cassert>
#include <cstdarg>
#include <thread>

#include "mainwindow.h"

#include "capture/animated-png.h"
#include "../../core/emu.h"
#include "../../core/cpu.h"
#include "../../core/control.h"
#include "../../core/link.h"
#include "../../core/debug/stepping.h"

EmuThread *emu_thread = Q_NULLPTR;

void gui_emu_sleep(unsigned long microseconds) {
    QThread::usleep(microseconds);
}

void gui_do_stuff(void) {
    emu_thread->doStuff();
}

static void gui_console_vprintf(const char *fmt, va_list ap) {
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->consoleStr(str);
}

void gui_console_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    gui_console_vprintf(fmt, ap);

    va_end(ap);
}

static void gui_console_err_vprintf(const char *fmt, va_list ap) {
    QString str;
    str.vsprintf(fmt, ap);
    emu_thread->consoleErrStr(str);
}

void gui_console_err_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    gui_console_err_vprintf(fmt, ap);

    va_end(ap);
}

void gui_debugger_send_command(int reason, uint32_t addr) {
    emu_thread->sendDebugCommand(reason, addr);
}

void gui_lcd_update(void) {
    emu_thread->drawLcd();
}

void gui_debugger_raise_or_disable(bool entered) {
    if (entered) {
        emu_thread->raiseDebugger();
    } else {
        emu_thread->disableDebugger();
    }
}

void throttle_timer_wait(void) {
    emu_thread->throttleTimerWait();
}

EmuThread::EmuThread(QObject *p) : QThread(p) {
    assert(emu_thread == Q_NULLPTR);
    emu_thread = this;
    speed = actualSpeed = 100;
    lastTime = std::chrono::steady_clock::now();
    connect(&guiTimer, SIGNAL(timeout()), this, SLOT(sendUpdates()));
    guiTimer.start();
    guiTimer.setInterval(600);
}

void EmuThread::reset() {
    doReset = true;
}

void EmuThread::drawLcd() {
    if (skip) {
        skip--;
    } else {
        skip = frameskip;
        if (mode) {
            memcpy(lcd_gui_buffer, spi.display, sizeof(spi.display));
        } else {
            lcd_drawframe(lcd_gui_buffer, lcd.control & 1 << 11 ? lcd.data : nullptr, lcd.data_end, lcd.control, LCD_SIZE);
        }
#ifdef PNG_WRITE_APNG_SUPPORTED
        apng_add_frame(lcd_gui_buffer);
#endif
        emit updateLcd();
    }
}

void EmuThread::setMode(bool state) {
    mode = state;
}

void EmuThread::setFrameskip(int value) {
    frameskip = value;
    skip = value;
}

void EmuThread::setEmuSpeed(int value) {
    speed = value;
}

void EmuThread::setThrottleMode(bool throttled) {
    throttleOn = throttled;
}

void EmuThread::setDebugMode(bool state) {
    enterDebugger = state;
    if (inDebugger && !state) {
        close_debugger();
        debug_clear_temp_break();
    }
}

void EmuThread::send(const QStringList &list, unsigned int location) {
    enterSendState = true;
    vars = list;
    sendLoc = location;
}

void EmuThread::receive() {
    enterReceiveState = true;
}

void EmuThread::receiveDone() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.notify_all();
}

void EmuThread::setRunUntilMode() {
    debug_set_run_until();
    close_debugger();
}

void EmuThread::setDebugStepInMode() {
    debug_set_step_in();
    close_debugger();
}

void EmuThread::setDebugStepOverMode() {
    debug_set_step_over();
    close_debugger();
}

void EmuThread::setDebugStepNextMode() {
    debug_set_step_next();
    close_debugger();
}

void EmuThread::setDebugStepOutMode() {
    debug_set_step_out();
    close_debugger();
}

// Called occasionally, only way to do something in the same thread the emulator runs in.
void EmuThread::doStuff() {
    const std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();
    lastTime += std::chrono::steady_clock::now() - cur_time;

    if (doReset) {
        cpu.events |= EVENT_RESET;
        doReset = false;
    }

    if (enterSaveImage) {
        bool success = emu_save(image.toStdString().c_str());
        emit saved(success);
        enterSaveImage = false;
    }

    if (enterSaveRom) {
        bool success = emu_save_rom(romExportPath.toStdString().c_str());
        emit saved(success);
        enterSaveRom = false;
    }

    if (debugger.bufferPos) {
        debugger.buffer[debugger.bufferPos] = '\0';
        consoleStr(QString(debugger.buffer));
        debugger.bufferPos = 0;
    }

    if (debugger.bufferErrPos) {
        debugger.bufferErr[debugger.bufferErrPos] = '\0';
        consoleErrStr(QString(debugger.bufferErr));
        debugger.bufferErrPos = 0;
    }

    if (enterSendState) {
        sendFiles();
        enterSendState = false;
    }

    if (enterReceiveState) {
        std::unique_lock<std::mutex> lock(mutex);
        emit receiveReady();
        cv.wait(lock);
        enterReceiveState = false;
    }

    if (enterDebugger) {
        open_debugger(DBG_USER, 0);
        enterDebugger = false;
    }
}

void EmuThread::sendFiles() {
    const int fileNum = vars.size();

    for (int i = 0; i < fileNum; i++) {
        const QString &f = vars.at(i);
        emit sentFile(f, sendVariableLink(f.toUtf8(), sendLoc));
    }

    emit sentFile(QString(), LINK_GOOD);
}

void EmuThread::sendUpdates() {
    if (!control.off) {
        emuFps = 24e6 / (lcd.PCD * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * (lcd.VSW + lcd.VBP + lcd.LPP + lcd.VFP));
        emit sendGuiUpdates(actualSpeed, emuFps / (frameskip + 1));
    }
}

void EmuThread::setActualSpeed(int value) {
    if (!control.off && actualSpeed != value) {
        actualSpeed = value;
    }
}

void EmuThread::throttleTimerWait() {
    if (!speed) {
        setActualSpeed(0);
        while(!speed) {
            QThread::usleep(10000);
        }
        return;
    }
    std::chrono::duration<int, std::ratio<100, 60>> unit(1);
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = lastTime + interval;
    if (throttleOn && cur_time < next_time) {
        setActualSpeed(speed);
        lastTime = next_time;
        std::this_thread::sleep_until(next_time);
    } else {
        if (lastTime != cur_time) {
            setActualSpeed(unit / (cur_time - lastTime));
            lastTime = cur_time;
        }
        std::this_thread::yield();
    }
}

void EmuThread::run() {
    setTerminationEnabled();

    bool reset = !enterRestore;
    bool success = emu_load(rom.toStdString().c_str(), enterRestore ? image.toStdString().c_str() : NULL);

    if (enterRestore) {
        emit restored(success);
    } else {
        emit started(success);
    }

    enterRestore = false;

    if (success) {
        emu_loop(reset);
    }
    emit stopped();
}

bool EmuThread::stop() {

    if (!isRunning()) {
        return true;
    }

    lcd_gui_callback = NULL;
    lcd_gui_buffer = NULL;
    guiTimer.stop();

    exiting = true;
    cpu.next = 0;

    if (!this->wait(200)) {
        terminate();
        if (!this->wait(200)) {
            return false;
        }
    }

    return true;
}

void EmuThread::load() {
    if (!stop()) {
        return;
    }

    start();
    return;
}

bool EmuThread::restore(const QString &path) {
    image = QDir::toNativeSeparators(path);
    enterRestore = true;

    if (!stop()) {
        return false;
    }

    start();
    return true;
}

void EmuThread::saveImage(const QString &path) {
    image = QDir::toNativeSeparators(path);
    enterSaveImage = true;
}

void EmuThread::saveRom(const QString &path) {
    romExportPath = QDir::toNativeSeparators(path);
    enterSaveRom = true;
}
