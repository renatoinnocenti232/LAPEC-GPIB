/**
 * @file exceptionhandler.h
 * @brief Captura global de exceções não tratadas.
 */

#pragma once

#include <QObject>
#include <exception>

/**
 * @brief Singleton que instala um handler para std::terminate.
 * 
 * Quando uma exceção não é capturada, gera log com stack trace e emite sinal.
 */
class ExceptionHandler : public QObject {
    Q_OBJECT
public:
    static ExceptionHandler& instance();

    /**
     * @brief Instala o handler global.
     */
    void install();

    /**
     * @brief Processa uma exceção capturada.
     * @param e Exceção derivada de std::exception.
     */
    void handleException(const std::exception& e);

signals:
    /**
     * @brief Emitido quando uma exceção não tratada é capturada.
     * @param message Mensagem da exceção.
     * @param stackTrace Stack trace (se disponível).
     */
    void unhandledException(const QString& message, const QString& stackTrace);

private:
    ExceptionHandler();
    static void customTerminate();
};