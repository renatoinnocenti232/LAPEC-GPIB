#include "commandhistory.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include "logger.h"

CommandHistory::CommandHistory(QObject *parent) : QObject(parent) {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    filePath_ = dataDir + "/command_history.txt";
    load();
}

void CommandHistory::addCommand(const QString& cmd) {
    if (cmd.trimmed().isEmpty()) return;
    history_.removeAll(cmd);
    history_.prepend(cmd);
    while (history_.size() > MAX_HISTORY) {
        history_.removeLast();
    }
    save();
}

QStringList CommandHistory::commands() const {
    return history_;
}

QCompleter* CommandHistory::createCompleter(QObject *parent) const {
    auto *completer = new QCompleter(history_, parent);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    return completer;
}

void CommandHistory::load() {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::instance().warning("Não foi possível carregar histórico de comandos.");
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.isEmpty()) history_.append(line);
    }
}

void CommandHistory::save() {
    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance().warning("Não foi possível salvar histórico de comandos.");
        return;
    }
    QTextStream out(&file);
    for (const QString& cmd : history_) {
        out << cmd << "\n";
    }
}