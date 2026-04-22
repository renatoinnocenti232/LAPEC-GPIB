#include "restapi.h"
#include <QJsonDocument>
#include <QUrlQuery>
#include <QRegularExpression>
#include "gpib_config.h"
#include "logger.h"

RestApi::RestApi(quint16 port, QObject *parent) : QObject(parent) {
    server_ = new QTcpServer(this);
    if (server_->listen(QHostAddress::Any, port)) {
        Logger::instance().info(QString("Servidor REST ouvindo na porta %1").arg(port));
        connect(server_, &QTcpServer::newConnection, this, &RestApi::onNewConnection);
    } else {
        Logger::instance().error("Falha ao iniciar servidor REST.");
    }
}

RestApi::~RestApi() {
    parar();
}

bool RestApi::iniciar() {
    return server_->isListening();
}

void RestApi::parar() {
    server_->close();
    qDeleteAll(clients_);
    clients_.clear();
}

void RestApi::onNewConnection() {
    QTcpSocket *socket = server_->nextPendingConnection();
    if (!socket) return;

    if (localhostOnly_) {
        QHostAddress clientAddr = socket->peerAddress();
        if (clientAddr != QHostAddress::LocalHost && clientAddr != QHostAddress::LocalHostIPv6) {
            socket->write("HTTP/1.1 403 Forbidden\r\n\r\nAccess denied: localhost only.");
            socket->disconnectFromHost();
            socket->deleteLater();
            Logger::instance().warning(QString("Tentativa de conexão REST de %1 bloqueada.").arg(clientAddr.toString()));
            return;
        }
    }

    connect(socket, &QTcpSocket::readyRead, this, &RestApi::onReadyRead);
    clients_ << socket;
}

void RestApi::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QString request = QString::fromUtf8(socket->readAll());
    handleRequest(socket, request);
}

void RestApi::handleRequest(QTcpSocket *socket, const QString& request) {
    // Extrai método e path
    QRegularExpression re("^(\\w+) ([^\\s]+)");
    auto match = re.match(request);
    if (!match.hasMatch()) {
        sendResponse(socket, {{"error", "Bad request"}}, 400);
        return;
    }
    QString method = match.captured(1);
    QString path = match.captured(2);

    // Extrai corpo JSON (se houver)
    QString body;
    int bodyStart = request.indexOf("\r\n\r\n");
    if (bodyStart >= 0) {
        body = request.mid(bodyStart + 4);
    }

    // Roteamento simples
    if (path == "/api/command" && method == "POST") {
        QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
        if (!doc.isObject()) {
            sendResponse(socket, {{"error", "JSON inválido"}}, 400);
            return;
        }
        QJsonObject obj = doc.object();
        if (!authenticate(obj)) {
            sendResponse(socket, {{"error", "Chave de API inválida"}}, 401);
            logRemoteAccess(socket->peerAddress().toString(), "AUTH_FAILED");
            return;
        }

        QString cmd = obj["command"].toString();
        int address = obj["address"].toInt();
        int board = obj["board"].toInt(0);
        logRemoteAccess(socket->peerAddress().toString(), cmd);

        QJsonObject response;
        try {
            auto instr = Gpib::GpibManager::instance().getInstrumento(address, board);
            if (cmd == "write") {
                instr->enviar(obj["data"].toString().toStdString());
                response["status"] = "ok";
            } else if (cmd == "query") {
                std::string result = instr->query(obj["query"].toString().toStdString());
                response["result"] = QString::fromStdString(result);
            } else if (cmd == "read") {
                std::string result = instr->ler();
                response["result"] = QString::fromStdString(result);
            } else {
                response["error"] = "Comando desconhecido";
            }
        } catch (const std::exception& e) {
            response["error"] = e.what();
        }
        sendResponse(socket, response);
    } else {
        sendResponse(socket, {{"error", "Not found"}}, 404);
    }
}

bool RestApi::authenticate(const QJsonObject& obj) {
    if (apiKey_.isEmpty()) return true;
    return obj["apiKey"].toString() == apiKey_;
}

void RestApi::sendResponse(QTcpSocket *socket, const QJsonObject& response, int statusCode) {
    QJsonDocument doc(response);
    QByteArray data = doc.toJson();
    QString header = QString("HTTP/1.1 %1 OK\r\n"
                             "Content-Type: application/json\r\n"
                             "Content-Length: %2\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "\r\n").arg(statusCode).arg(data.size());
    socket->write(header.toUtf8());
    socket->write(data);
    socket->disconnectFromHost();
}

void RestApi::logRemoteAccess(const QString& clientIp, const QString& command) {
    Logger::instance().info(QString("API REST [%1]: %2").arg(clientIp, command));
}