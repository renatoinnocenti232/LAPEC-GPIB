#ifndef EXCEPTIONHANDLER_H
#define EXCEPTIONHANDLER_H

#include <QObject>
#include <exception>

class ExceptionHandler : public QObject {
    Q_OBJECT
public:
    static ExceptionHandler& instance();

    // Instala o handler global de exceções
    void install();

    // Registra exceção não tratada
    void handleException(const std::exception& e);

signals:
    void unhandledException(const QString& message, const QString& stackTrace);

private:
    ExceptionHandler();
    static void customTerminate();
};

#endif