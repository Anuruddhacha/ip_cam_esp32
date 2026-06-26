#include "MainWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QAbstractSocket>

namespace {
// Default relay viewer endpoint. Change to your own relay host.
constexpr const char *DEFAULT_URL =
    "wss://ipcamserver-bdf1an9v.b4a.run/view";
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    // ---- Top bar: URL + connect button ----
    auto *bar = new QHBoxLayout();
    bar->addWidget(new QLabel("Relay:", this));
    m_urlEdit = new QLineEdit(DEFAULT_URL, this);
    bar->addWidget(m_urlEdit, 1);
    m_connectBtn = new QPushButton("Connect", this);
    bar->addWidget(m_connectBtn);
    root->addLayout(bar);

    // ---- Status line ----
    m_statusLabel = new QLabel(this);
    root->addWidget(m_statusLabel);

    // ---- Video panel ----
    m_videoLabel = new QLabel("No video", this);
    m_videoLabel->setMinimumSize(320, 240);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("background:#000; color:#888; border:1px solid #333;");
    root->addWidget(m_videoLabel, 1);

    // ---- FPS readout ----
    m_fpsLabel = new QLabel("0 fps", this);
    root->addWidget(m_fpsLabel);

    setCentralWidget(central);
    setWindowTitle("ESP32-CAM Viewer");
    resize(720, 580);
    setStatus("Disconnected", "#e74c3c");

    // ---- Signals ----
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &MainWindow::toggleConnection);
    connect(&m_socket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(&m_socket, &QWebSocket::binaryMessageReceived, this, &MainWindow::onBinaryMessage);
    connect(&m_socket, &QWebSocket::errorOccurred, this, &MainWindow::onError);

    // ---- FPS timer: report and reset every second ----
    auto *fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, [this]() {
        m_fpsLabel->setText(QString::number(m_frameCount) + " fps");
        m_frameCount = 0;
    });
    fpsTimer->start(1000);
}

void MainWindow::toggleConnection() {
    const bool active = m_socket.state() != QAbstractSocket::UnconnectedState;
    if (active || m_wantConnected) {
        m_wantConnected = false;
        m_socket.close();
    } else {
        m_wantConnected = true;
        setStatus("Connecting...", "#e0a000");
        m_socket.open(QUrl(m_urlEdit->text().trimmed()));
    }
}

void MainWindow::onConnected() {
    m_connectBtn->setText("Disconnect");
    setStatus("Connected", "#2ecc71");
}

void MainWindow::onDisconnected() {
    m_connectBtn->setText("Connect");
    if (m_wantConnected) {
        setStatus("Disconnected - reconnecting...", "#e0a000");
        QTimer::singleShot(2000, this, [this]() {
            if (m_wantConnected) m_socket.open(QUrl(m_urlEdit->text().trimmed()));
        });
    } else {
        setStatus("Disconnected", "#e74c3c");
    }
}

void MainWindow::onBinaryMessage(const QByteArray &message) {
    QImage img;
    if (!img.loadFromData(message)) {  // auto-detects JPEG
        return;
    }
    m_videoLabel->setPixmap(QPixmap::fromImage(img).scaled(
        m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ++m_frameCount;
}

void MainWindow::onError() {
    setStatus("Error: " + m_socket.errorString(), "#e74c3c");
}

void MainWindow::setStatus(const QString &text, const QString &color) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString("color:%1; font-weight:bold;").arg(color));
}
