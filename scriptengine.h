#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QJSEngine>
#include <QFutureWatcher>
#include "gpib_config.h"

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    explicit ScriptEngine(QObject *parent = nullptr);

    // Expõe funções para JavaScript
    Q_INVOKABLE QString enviarComando(int endereco, int placa, const QString& cmd);
    Q_INVOKABLE QString consultar(int endereco, int placa, const QString& query);
    Q_INVOKABLE void aguardar(int ms);
    Q_INVOKABLE void log(const QString& msg);

    // Execução assíncrona (agora segura para QJSEngine)
    void executarScriptAsync(const QString& script);
    void executarScript(const QString& script); // síncrono na thread atual (cuidado)
    void executarArquivo(const QString& caminho);

signals:
    void scriptOutput(const QString& text);
    void scriptFinished();

private slots:
    void onAsyncFinished();

private:
    QJSEngine engine_;
    QFutureWatcher<void> asyncWatcher_;  // não usado mais com a nova abordagem, mas mantido para compatibilidade
    std::shared_ptr<Gpib::InstrumentoMestre> obterInstrumento(int endereco, int placa);
};

#endif // SCRIPTENGINE_H