#include <xrh/core/shutdown_signal.h>

#include <windows.h>

#include <atomic>

namespace xrh {

namespace {

std::atomic_bool gShutdownRequested{false};

BOOL WINAPI consoleCtrlHandler(DWORD controlType) {
    switch (controlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            gShutdownRequested.store(true, std::memory_order_relaxed);
            return TRUE;
        default:
            return FALSE;
    }
}

}  // namespace

bool ShutdownSignal::install() {
    reset();
    installed_ = SetConsoleCtrlHandler(consoleCtrlHandler, TRUE) == TRUE;
    return installed_;
}

void ShutdownSignal::uninstall() {
    if (installed_) {
        SetConsoleCtrlHandler(consoleCtrlHandler, FALSE);
        installed_ = false;
    }
}

bool ShutdownSignal::requested() const {
    return gShutdownRequested.load(std::memory_order_relaxed);
}

void ShutdownSignal::reset() {
    gShutdownRequested.store(false, std::memory_order_relaxed);
}

}  // namespace xrh
