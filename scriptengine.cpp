#include "scriptengine.h"
#include <QThread>
#include <QFile>
#include "logger.h"
#include "gpib_config.h"

ScriptEngine::ScriptEngine(QObject *parent) : QObject(parent) {
    // Expõe este objeto como "gpib"
    QJSValue obj = engine_.newQObject(this);
    engine_.globalObject().setProperty("gpib", obj);
}

void ScriptEngine::executarScript(const QString& script) {
    QJSValue result = engine_.evaluate(script);
    if (result.isError()) {
        QString err = QString("Script error: %1").arg(result.toString());
        Logger::instance().error(err);
        emit scriptOutput(err);
    }
}

void ScriptEngine::executarArquivo(const QString& caminho) {
    QFile file(caminho);
    if (!file.open(QIODevice::ReadOnly)) {
        emit scriptOutput("Não foi possível abrir o arquivo: " + caminho);
        return;
    }
    QString script = file.readAll();
    executarScript(script);
}

QString ScriptEngine::enviarComando(int endereco, const QString& cmd) {
    try {
        auto instr = obterInstrumento(endereco);
        instr->enviar(cmd.toStdString());
        return "";
    } catch (const std::exception& e) {
        return QString("Erro: %1").arg(e.what());
    }
}

QString ScriptEngine::consultar(int endereco, const QString& query) {
    try {
        auto instr = obterInstrumento(endereco);
        return QString::fromStdString(instr->query(query.toStdString()));
    } catch (const std::exception& e) {
        return QString("Erro: %1").arg(e.what());
    }
}

void ScriptEngine::aguardar(int ms) {
    QThread::msleep(ms);
}

void ScriptEngine::log(const QString& msg) {
    Logger::instance().info(msg);
    emit scriptOutput(msg);
}

std::shared_ptr<Gpib::InstrumentoMestre> ScriptEngine::obterInstrumento(int endereco) {
    return Gpib::GpibManager::instance().getInstrumento(endereco);
}