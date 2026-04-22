#include "websocketserver.h"
#include <QJsonDocument>
#include <QHostAddress>
#include <QtConcurrent/QtConcurrent>
#include "gpib_config.h"
#include "logger.h"
#include "jsonschema.h"

WebSocketServer::WebSocketServer(quint16 port, bool localhostOnly, QObject *parent)
    : QObject(parent)
    , server_(new QWebSocketServer("GpibMaestro", QWebSocketServer::NonSecureMode, this))
    , localhostOnly_(localhostOnly)
{
    QHostAddress listenAddr = localhostOnly ? QHostAddress::LocalHost : QHostAddress::Any;
    if (server_->listen(listenAddr, port)) {
        Logger::instance().info(QString("Servidor WebSocket ouvindo na porta %1").arg(port));
        connect(server_, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
    } else {
        Logger::instance().error("Falha ao iniciar servidor WebSocket: " + server_->errorString());
    }
}

bool WebSocketServer::iniciar() { return server_->isListening(); }

void WebSocketServer::parar() {
    server_->close();
    qDeleteAll(clients_);
    clients_.clear();
}

void WebSocketServer::setApiKey(const QString& key) {
    apiKey_ = key;
}

void WebSocketServer::setLocalhostOnly(bool enable) {
    localhostOnly_ = enable;
    // Não altera o endereço de escuta após iniciado
}

void WebSocketServer::onNewConnection() {
    QWebSocket *socket = server_->nextPendingConnection();
    
    // Verifica se é conexão local (se restrito)
    if (localhostOnly_ && socket->peerAddress() != QHostAddress::LocalHost
                        && socket->peerAddress() != QHostAddress::LocalHostIPv6) {
        Logger::instance().warning("Conexão WebSocket recusada de " + socket->peerAddress().toString());
        socket->close(QWebSocketProtocol::CloseCodePolicyViolated, "Apenas localhost permitido");
        socket->deleteLater();
        return;
    }

    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onSocketDisconnected);
    clients_ << socket;
    Logger::instance().info("Cliente WebSocket conectado: " + socket->peerAddress().toString());
}

bool WebSocketServer::autenticar(QWebSocket *client, const QJsonObject& obj) {
    if (apiKey_.isEmpty()) return true;

    QString key = obj["api_key"].toString();
    if (key != apiKey_) {
        QJsonObject error;
        error["status"] = "error";
        error["error"] = "API key inválida";
        client->sendTextMessage(QJsonDocument(error).toJson());
        return false;
    }
    return true;
}

void WebSocketServer::onTextMessageReceived(const QString& message) {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        client->sendTextMessage(JsonSchemaValidator::createErrorResponse("JSON inválido: " + parseError.errorString()));
        return;
    }
    if (!doc.isObject()) {
        client->sendTextMessage(JsonSchemaValidator::createErrorResponse("Esperado objeto JSON"));
        return;
    }

    QJsonObject obj = doc.object();

    if (!autenticar(client, obj)) return;

    QString validationError;
    if (!JsonSchemaValidator::validateCommand(obj, validationError)) {
        client->sendTextMessage(JsonSchemaValidator::createErrorResponse(validationError));
        return;
    }

    // MELHORIA: processar comando em thread separada para não bloquear o servidor
    QtConcurrent::run([this, client, obj]() {
        processarComando(client, obj);
    });
}

void WebSocketServer::processarComando(QWebSocket *client, const QJsonObject& obj) {
    QString cmd = obj["command"].toString();
    int endereco = obj["address"].toInt();
    int placa = obj.value("board").toInt(0);

    QJsonObject resposta;
    resposta["command"] = cmd;

    try {
        auto instr = Gpib::GpibManager::instance().getInstrumento(endereco, placa);
        if (cmd == "write") {
            QString data = obj["data"].toString();
            instr->enviar(data.toStdString());
            resposta["status"] = "ok";
        } else if (cmd == "query") {
            QString query = obj["query"].toString();
            std::string result = instr->query(query.toStdString());
            resposta["result"] = QString::fromStdString(result);
            resposta["status"] = "ok";
        } else if (cmd == "read") {
            std::string result = instr->ler();
            resposta["result"] = QString::fromStdString(result);
            resposta["status"] = "ok";
        } else if (cmd == "batch") {
            // Implementação básica de batch
            QJsonArray commands = obj["commands"].toArray();
            QJsonArray results;
            for (const QJsonValue& val : commands) {
                QJsonObject sub = val.toObject();
                // Processamento similar, omitido por brevidade (pode ser expandido)
            }
            resposta["status"] = "ok";
            resposta["results"] = results;
        } else {
            resposta = JsonSchemaValidator::createErrorResponse("Comando desconhecido");
        }
    } catch (const std::exception& e) {
        resposta = JsonSchemaValidator::createErrorResponse(e.what());
    }

    // Enviar resposta de volta na thread principal (QWebSocket não é thread-safe)
    QMetaObject::invokeMethod(client, [client, resposta]() {
        client->sendTextMessage(QJsonDocument(resposta).toJson());
    });
}

void WebSocketServer::onSocketDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        clients_.removeAll(client);
        client->deleteLater();
        Logger::instance().info("Cliente WebSocket desconectado.");
    }
}