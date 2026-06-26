#pragma once

#include <QMainWindow>
#include <QtWebSockets/QWebSocket>

class QLabel;
class QLineEdit;
class QPushButton;

// Desktop viewer for the ESP32-CAM relay.
// Connects to the relay's /view WebSocket endpoint and renders each incoming
// binary message as a JPEG frame.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void toggleConnection();
    void onConnected();
    void onDisconnected();
    void onBinaryMessage(const QByteArray &message);
    void onError();

private:
    void setStatus(const QString &text, const QString &color);

    QLineEdit  *m_urlEdit     = nullptr;
    QPushButton*m_connectBtn  = nullptr;
    QLabel     *m_statusLabel = nullptr;
    QLabel     *m_videoLabel  = nullptr;
    QLabel     *m_fpsLabel    = nullptr;

    QWebSocket m_socket;
    bool m_wantConnected = false;  // user intent, drives auto-reconnect
    int  m_frameCount = 0;         // frames since last FPS tick
};
