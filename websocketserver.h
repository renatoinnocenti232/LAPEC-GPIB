#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>

class WebSocketServer : public QObject {
    Q_OBJECT
public:
    explicit WebSocketServer(quint16 port = 12345, bool localhostOnly = false, QObject *parent = nullptr);
    bool iniciar();
    void parar();
    void setApiKey(const QString& key);
    void setLocalhostOnly(bool enable);

signals:
    void mensagemRecebida(const QString& mensagem);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onSocketDisconnected();

private:
    QWebSocketServer *server_;
    QList<QWebSocket*> clients_;  // mantido, mas com gerenciamento seguro via deleteLater
    QString apiKey_;
    bool localhostOnly_ = false;

    void processarComando(QWebSocket *client, const QJsonObject& obj);
    bool autenticar(QWebSocket *client, const QJsonObject& obj);
};

#endif // WEBSOCKETSERVER_H