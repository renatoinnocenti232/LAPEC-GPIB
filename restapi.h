/**
 * @file restapi.h
 * @brief Servidor REST para controle remoto via HTTP.
 */

#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>

/**
 * @brief Servidor HTTP simples com endpoints REST para controle GPIB.
 */
class RestApi : public QObject {
    Q_OBJECT
public:
    explicit RestApi(quint16 port = 8080, QObject *parent = nullptr);
    ~RestApi();

    bool iniciar();
    void parar();

    void setApiKey(const QString& key) { apiKey_ = key; }
    void setLocalhostOnly(bool enabled) { localhostOnly_ = enabled; }

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QTcpServer *server_;
    QList<QTcpSocket*> clients_;
    QString apiKey_;
    bool localhostOnly_ = true;

    void handleRequest(QTcpSocket *socket, const QString& request);
    bool authenticate(const QJsonObject& obj);
    void sendResponse(QTcpSocket *socket, const QJsonObject& response, int statusCode = 200);
    void logRemoteAccess(const QString& clientIp, const QString& command);
};