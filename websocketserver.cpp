#include "websocketserver.h"
#include <QJsonDocument>
#include "gpib_config.h"
#include "logger.h"

WebSocketServer::WebSocketServer(quint16 port, QObject *parent)
    : QObject(parent), server_(new QWebSocketServer("GpibMaestro", QWebSocketServer::NonSecureMode, this)) {
    if (server_->listen(QHostAddress::Any, port)) {
        Logger::instance().info(QString("Servidor WebSocket ouvindo na porta %1").arg(port));
        connect(server_, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
    } else {
        Logger::instance().error("Falha ao iniciar servidor WebSocket.");
    }
}

bool WebSocketServer::iniciar() { return server_->isListening(); }

void WebSocketServer::parar() {
    server_->close();
    qDeleteAll(clients_);
    clients_.clear();
}

void WebSocketServer::onNewConnection() {
    QWebSocket *socket = server_->nextPendingConnection();
    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onSocketDisconnected);
    clients_ << socket;
}

void WebSocketServer::onTextMessageReceived(const QString& message) {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        client->sendTextMessage("{\"error\":\"JSON inválido\"}");
        return;
    }
    processarComando(client, doc.object());
}

void WebSocketServer::processarComando(QWebSocket *client, const QJsonObject& obj) {
    QString cmd = obj["command"].toString();
    int endereco = obj["address"].toInt();
    QJsonObject resposta;
    resposta["command"] = cmd;

    try {
        auto instr = Gpib::GpibManager::instance().getInstrumento(endereco);
        if (cmd == "write") {
            QString data = obj["data"].toString();
            instr->enviar(data.toStdString());
            resposta["status"] = "ok";
        } else if (cmd == "query") {
            QString query = obj["query"].toString();
            std::string result = instr->query(query.toStdString());
            resposta["result"] = QString::fromStdString(result);
        } else if (cmd == "read") {
            std::string result = instr->ler();
            resposta["result"] = QString::fromStdString(result);
        } else {
            resposta["error"] = "Comando desconhecido";
        }
    } catch (const std::exception& e) {
        resposta["error"] = e.what();
    }

    client->sendTextMessage(QJsonDocument(resposta).toJson());
}

void WebSocketServer::onSocketDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    clients_.removeAll(client);
    client->deleteLater();
}