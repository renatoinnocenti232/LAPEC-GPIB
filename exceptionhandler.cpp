#include "exceptionhandler.h"
#include "logger.h"
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <cxxabi.h>
#include <execinfo.h>
#endif

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

static QString getStackTrace() {
    QString stack;
#ifdef Q_OS_WIN
    void* stackTrace[64];
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    WORD frames = CaptureStackBackTrace(0, 64, stackTrace, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    for (WORD i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stackTrace[i]), 0, symbol);
        stack += QString::fromLatin1(symbol->Name) + "\n";
    }
    free(symbol);
#else
    void* trace[32];
    int size = backtrace(trace, 32);
    char** symbols = backtrace_symbols(trace, size);
    for (int i = 0; i < size; ++i) {
        stack += QString::fromLatin1(symbols[i]) + "\n";
    }
    free(symbols);
#endif
    return stack;
}

void ExceptionHandler::handleException(const std::exception& e) {
    QString msg = QString("Exceção não tratada: %1").arg(e.what());
    QString stack = getStackTrace();
    Logger::instance().error(msg + "\nStack trace:\n" + stack);
    emit unhandledException(msg, stack);
}