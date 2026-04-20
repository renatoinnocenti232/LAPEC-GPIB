#include "logger.h"
#include <QDebug>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    // Por padrão, loga apenas em console
    stream_.setString(nullptr);
}

void Logger::setLogFile(const QString& filepath) {
    QMutexLocker locker(&mutex_);
    if (logFile_.isOpen()) logFile_.close();
    logFile_.setFileName(filepath);
    if (!logFile_.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Não foi possível abrir arquivo de log:" << filepath;
    } else {
        stream_.setDevice(&logFile_);
    }
}

QString Logger::levelToString(Level level) {
    switch (level) {
    case Level::Debug:   return "DEBUG";
    case Level::Info:    return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error:   return "ERROR";
    default: return "";
    }
}

void Logger::log(Level level, const QString& message) {
    QMutexLocker locker(&mutex_);
    QString formatted = QString("[%1] [%2] %3")
                        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
                        .arg(levelToString(level))
                        .arg(message);
    
    // Saída no console (stderr para erros)
    if (level == Level::Error) {
        qCritical().noquote() << formatted;
    } else {
        qDebug().noquote() << formatted;
    }

    // Arquivo
    if (logFile_.isOpen()) {
        stream_ << formatted << Qt::endl;
    }

    emit newLogMessage(formatted);
}