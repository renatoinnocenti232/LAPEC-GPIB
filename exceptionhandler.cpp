#include "exceptionhandler.h"
#include "logger.h"
#include <QString>
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>

ExceptionHandler& ExceptionHandler::instance() {
    static ExceptionHandler inst;
    return inst;
}

ExceptionHandler::ExceptionHandler() {}

void ExceptionHandler::install() {
    std::set_terminate(customTerminate);
}

void ExceptionHandler::customTerminate() {
    auto eptr = std::current_exception();
    if (eptr) {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
            instance().handleException(e);
        } catch (...) {
            Logger::instance().error("Terminate chamado com exceção desconhecida.");
        }
    }
    std::abort();
}

void ExceptionHandler::handleException(const std::exception& e) {
    QString msg = QString("Exceção não tratada: %1").arg(e.what());
    // Tenta obter stack trace
    void* trace[32];
    int size = backtrace(trace, 32);
    char** symbols = backtrace_symbols(trace, size);
    QString stack;
    for (int i = 0; i < size; ++i) {
        stack += QString::fromLatin1(symbols[i]) + "\n";
    }
    free(symbols);
    Logger::instance().error(msg + "\nStack trace:\n" + stack);
    emit unhandledException(msg, stack);
}