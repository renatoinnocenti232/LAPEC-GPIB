#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>

class WebSocketServer : public QObject {
    Q_OBJECT
public:
    explicit WebSocketServer(quint16 port = 12345, QObject *parent = nullptr);
    bool iniciar();
    void parar();

signals:
    void mensagemRecebida(const QString& mensagem);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onSocketDisconnected();

private:
    QWebSocketServer *server_;
    QList<QWebSocket*> clients_;
    void processarComando(QWebSocket *client, const QJsonObject& obj);
};

#endif