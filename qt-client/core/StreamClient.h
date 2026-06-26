#pragma once

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QImage>
#include <QByteArray>
#include <QString>

// Owns the WebSocket connection to the relay's /view endpoint, handles
// auto-reconnect, and decodes each binary message into a QImage frame.
// Pure networking concern - no UI here.
class StreamClient : public QObject {
    Q_OBJECT

public:
    enum class State { Disconnected, Connecting, Connected, Reconnecting };
    Q_ENUM(State)

    explicit StreamClient(QObject *parent = nullptr);

    void setUrl(const QString &url);
    void setAutoReconnect(bool enabled);

    State state() const { return m_state; }
    bool isActive() const { return m_wantConnected; }

public slots:
    void start();   // begin connecting (and keep reconnecting if enabled)
    void stop();    // stop and disable reconnect

signals:
    void stateChanged(StreamClient::State state, const QString &message);
    void frameReceived(const QByteArray &jpeg, const QImage &image);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onBinaryMessage(const QByteArray &data);
    void onError();

private:
    void openSocket();
    void setState(State state, const QString &message);

    QWebSocket m_socket;
    QString    m_url;
    bool       m_autoReconnect = true;
    bool       m_wantConnected = false;
    State      m_state = State::Disconnected;
};
