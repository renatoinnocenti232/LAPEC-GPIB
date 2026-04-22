/**
 * @file logger.h
 * @brief Sistema de logging centralizado, thread-safe.
 */

#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

/**
 * @brief Singleton que gerencia logs da aplicação.
 * 
 * Permite redirecionar logs para console e arquivo simultaneamente.
 * Emite sinal para exibição em GUI.
 */
class Logger : public QObject {
    Q_OBJECT
public:
    enum class Level { Debug, Info, Warning, Error };

    static Logger& instance();

    /**
     * @brief Define o arquivo de log.
     * @param filepath Caminho completo do arquivo.
     */
    void setLogFile(const QString& filepath);

    /**
     * @brief Registra uma mensagem com nível específico.
     * @param level Nível da mensagem.
     * @param message Conteúdo.
     */
    void log(Level level, const QString& message);

    void debug(const QString& msg)   { log(Level::Debug, msg); }
    void info(const QString& msg)    { log(Level::Info, msg); }
    void warning(const QString& msg) { log(Level::Warning, msg); }
    void error(const QString& msg)   { log(Level::Error, msg); }

signals:
    /**
     * @brief Emitido quando uma nova mensagem é registrada.
     * @param formatted Mensagem formatada com timestamp e nível.
     */
    void newLogMessage(const QString& formatted);

private:
    Logger();
    QFile logFile_;
    QTextStream stream_;
    QMutex mutex_;
    QString levelToString(Level level);
};