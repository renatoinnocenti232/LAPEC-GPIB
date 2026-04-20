#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include <QJSEngine>
#include "gpib_config.h"

class ScriptEngine : public QObject {
    Q_OBJECT
public:
    explicit ScriptEngine(QObject *parent = nullptr);

    // Expõe funções para JavaScript
    Q_INVOKABLE QString enviarComando(int endereco, const QString& cmd);
    Q_INVOKABLE QString consultar(int endereco, const QString& query);
    Q_INVOKABLE void aguardar(int ms);
    Q_INVOKABLE void log(const QString& msg);

    void executarScript(const QString& script);
    void executarArquivo(const QString& caminho);

signals:
    void scriptOutput(const QString& text);

private:
    QJSEngine engine_;
    std::shared_ptr<Gpib::InstrumentoMestre> obterInstrumento(int endereco);
};

#endif