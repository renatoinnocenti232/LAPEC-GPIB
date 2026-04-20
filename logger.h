#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

class Logger : public QObject {
    Q_OBJECT
public:
    enum class Level { Debug, Info, Warning, Error };

    static Logger& instance();

    void setLogFile(const QString& filepath);
    void log(Level level, const QString& message);
    void debug(const QString& msg)   { log(Level::Debug, msg); }
    void info(const QString& msg)    { log(Level::Info, msg); }
    void warning(const QString& msg) { log(Level::Warning, msg); }
    void error(const QString& msg)   { log(Level::Error, msg); }

signals:
    void newLogMessage(const QString& formatted); // para GUI, se quiser

private:
    Logger();
    QFile logFile_;
    QTextStream stream_;
    QMutex mutex_;
    QString levelToString(Level level);
};

#endif