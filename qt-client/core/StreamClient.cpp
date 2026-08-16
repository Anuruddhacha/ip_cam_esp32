#include "core/StreamClient.h"

#include <QTimer>
#include <QAbstractSocket>

StreamClient::StreamClient(QObject *parent) : QObject(parent) {
    connect(&m_socket, &QWebSocket::connected,    this, &StreamClient::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &StreamClient::onDisconnected);
    connect(&m_socket, &QWebSocket::binaryMessageReceived, this, &StreamClient::onBinaryMessage);
    connect(&m_socket, &QWebSocket::errorOccurred, this, &StreamClient::onError);
}

void StreamClient::setUrl(const QString &url) {
    m_url = url.trimmed();
}

void StreamClient::setAutoReconnect(bool enabled) {
    m_autoReconnect = enabled;
}

void StreamClient::start() {
    if (m_url.isEmpty()) {
        emit errorOccurred("No relay URL set");
        return;
    }
    m_wantConnected = true;
    openSocket();
}

void StreamClient::stop() {
    m_wantConnected = false;
    m_socket.close();
    setState(State::Disconnected, "OFFLINE");
}

void StreamClient::openSocket() {
    setState(State::Connecting, "CONNECTING...");
    m_socket.open(QUrl(m_url));
}

void StreamClient::setState(State state, const QString &message) {
    m_state = state;
    emit stateChanged(state, message);
}

void StreamClient::onConnected() {
    setState(State::Connected, "LIVE");
}

void StreamClient::onDisconnected() {
    if (m_wantConnected && m_autoReconnect) {
        setState(State::Reconnecting, "RECONNECTING...");
        QTimer::singleShot(2000, this, [this]() {
            if (m_wantConnected) openSocket();
        });
    } else {
        m_wantConnected = false;
        setState(State::Disconnected, "OFFLINE");
    }
}

void StreamClient::onBinaryMessage(const QByteArray &data) {
    QImage image;
    if (!image.loadFromData(data)) {
        return;  // not a decodable frame; ignore
    }
    emit frameReceived(data, image);
}

void StreamClient::onError() {
    emit errorOccurred(m_socket.errorString());
}
