#include "MainWindow.h"
#include "VideoWidget.h"

#include <QApplication>
#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QAbstractSocket>

namespace {
constexpr const char *DEFAULT_URL =
    "wss://ipcamserver-bdf1an9v.b4a.run/view";
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    applyTheme();
    buildUi();
    buildToolbar();
    buildDock();
    buildStatusBar();

    setWindowTitle("ESP32-CAM  |  Industrial Viewer");
    resize(1100, 720);
    setConnectionState("OFFLINE", QColor(231, 76, 60));

    connect(&m_socket, &QWebSocket::connected,    this, &MainWindow::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(&m_socket, &QWebSocket::binaryMessageReceived, this, &MainWindow::onBinaryMessage);
    connect(&m_socket, &QWebSocket::errorOccurred, this, &MainWindow::onError);

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::tickStats);
    timer->start(1000);
}

void MainWindow::buildUi() {
    m_video = new VideoWidget(this);
    connect(m_video, &VideoWidget::doubleClicked, this, [this]() {
        m_fullscreenAct->toggle();
    });
    setCentralWidget(m_video);
}

void MainWindow::buildToolbar() {
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(22, 22));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    const QStyle *st = style();

    m_connectAct = tb->addAction(st->standardIcon(QStyle::SP_MediaPlay), "Connect");
    m_connectAct->setShortcut(QKeySequence("Ctrl+K"));
    connect(m_connectAct, &QAction::triggered, this, &MainWindow::toggleConnection);

    tb->addSeparator();

    m_snapshotAct = tb->addAction(st->standardIcon(QStyle::SP_DialogSaveButton), "Snapshot");
    m_snapshotAct->setShortcut(QKeySequence("Ctrl+S"));
    m_snapshotAct->setEnabled(false);
    connect(m_snapshotAct, &QAction::triggered, this, &MainWindow::takeSnapshot);

    m_recordAct = tb->addAction(st->standardIcon(QStyle::SP_DialogYesButton), "Record");
    m_recordAct->setCheckable(true);
    m_recordAct->setShortcut(QKeySequence("Ctrl+R"));
    m_recordAct->setEnabled(false);
    connect(m_recordAct, &QAction::toggled, this, &MainWindow::toggleRecording);

    tb->addSeparator();

    m_fullscreenAct = tb->addAction(st->standardIcon(QStyle::SP_TitleBarMaxButton), "Fullscreen");
    m_fullscreenAct->setCheckable(true);
    m_fullscreenAct->setShortcut(QKeySequence("F11"));
    connect(m_fullscreenAct, &QAction::toggled, this, &MainWindow::toggleFullScreen);
}

void MainWindow::buildDock() {
    auto *dock = new QDockWidget("Camera", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *panel = new QWidget(dock);
    auto *root = new QVBoxLayout(panel);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    // ---- Connection group ----
    auto *connGroup = new QGroupBox("Connection", panel);
    auto *form = new QFormLayout(connGroup);
    m_nameEdit = new QLineEdit("CAM 01", connGroup);
    m_urlEdit  = new QLineEdit(DEFAULT_URL, connGroup);
    m_autoReconnect = new QCheckBox("Auto-reconnect", connGroup);
    m_autoReconnect->setChecked(true);
    form->addRow("Name:", m_nameEdit);
    form->addRow("Relay URL:", m_urlEdit);
    form->addRow("", m_autoReconnect);
    root->addWidget(connGroup);

    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_video->setCameraName(t.isEmpty() ? "CAM" : t);
    });
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &MainWindow::toggleConnection);

    // ---- Stream info group ----
    auto *infoGroup = new QGroupBox("Stream", panel);
    auto *info = new QFormLayout(infoGroup);
    m_statusText  = new QLabel("OFFLINE", infoGroup);
    m_resLabel    = new QLabel("-", infoGroup);
    m_fpsLabel    = new QLabel("0", infoGroup);
    m_rateLabel   = new QLabel("0 kbps", infoGroup);
    m_framesLabel = new QLabel("0", infoGroup);
    info->addRow("Status:", m_statusText);
    info->addRow("Resolution:", m_resLabel);
    info->addRow("FPS:", m_fpsLabel);
    info->addRow("Bitrate:", m_rateLabel);
    info->addRow("Frames:", m_framesLabel);
    root->addWidget(infoGroup);

    root->addStretch(1);

    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::buildStatusBar() {
    statusBar()->showMessage("Ready");
}

void MainWindow::applyTheme() {
    qApp->setStyleSheet(R"(
        QMainWindow, QWidget { background: #1b1d22; color: #d7d9de; }
        QToolBar { background: #23262d; border: 0; padding: 4px; spacing: 4px; }
        QToolBar QToolButton { color: #d7d9de; padding: 6px 10px; border-radius: 4px; }
        QToolBar QToolButton:hover { background: #313640; }
        QToolBar QToolButton:checked { background: #2d6cdf; color: white; }
        QToolBar QToolButton:disabled { color: #6b6e76; }
        QDockWidget { titlebar-close-icon: none; color: #d7d9de; }
        QDockWidget::title { background: #23262d; padding: 6px; }
        QGroupBox { border: 1px solid #31343c; border-radius: 6px; margin-top: 12px; padding-top: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #9aa0aa; }
        QLineEdit { background: #14161a; border: 1px solid #31343c; border-radius: 4px; padding: 5px; color: #eaecf0; }
        QLineEdit:focus { border: 1px solid #2d6cdf; }
        QStatusBar { background: #23262d; color: #9aa0aa; }
        QStatusBar QLabel { padding: 0 8px; }
        QCheckBox { color: #c7cad1; }
    )");
}

void MainWindow::setConnectionState(const QString &text, const QColor &color) {
    if (m_statusText) {
        m_statusText->setText(text);
        m_statusText->setStyleSheet(QString("color:%1; font-weight:bold;").arg(color.name()));
    }
    statusBar()->showMessage(text);
}

void MainWindow::openConnection() {
    setConnectionState("CONNECTING...", QColor(230, 170, 0));
    m_video->setConnected(true);
    m_socket.open(QUrl(m_urlEdit->text().trimmed()));
}

void MainWindow::toggleConnection() {
    const bool active = m_socket.state() != QAbstractSocket::UnconnectedState;
    if (active || m_wantConnected) {
        m_wantConnected = false;
        m_socket.close();
    } else {
        m_wantConnected = true;
        m_video->setCameraName(m_nameEdit->text());
        openConnection();
    }
}

void MainWindow::onConnected() {
    m_connectAct->setText("Disconnect");
    m_connectAct->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_snapshotAct->setEnabled(true);
    m_recordAct->setEnabled(true);
    setConnectionState("LIVE", QColor(46, 204, 113));
}

void MainWindow::onDisconnected() {
    m_connectAct->setText("Connect");
    m_connectAct->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_snapshotAct->setEnabled(false);
    m_video->setConnected(false);

    if (m_recording) m_recordAct->setChecked(false);
    m_recordAct->setEnabled(false);

    if (m_wantConnected && m_autoReconnect->isChecked()) {
        setConnectionState("RECONNECTING...", QColor(230, 170, 0));
        QTimer::singleShot(2000, this, [this]() {
            if (m_wantConnected) openConnection();
        });
    } else {
        m_wantConnected = false;
        setConnectionState("OFFLINE", QColor(231, 76, 60));
    }
}

void MainWindow::onBinaryMessage(const QByteArray &data) {
    QImage img;
    if (!img.loadFromData(data)) return;

    m_video->setFrame(img);
    ++m_frameCount;
    ++m_totalFrames;
    m_bytesSinceTick += static_cast<quint64>(data.size());

    if (img.size() != m_lastSize) {
        m_lastSize = img.size();
        m_resLabel->setText(QString("%1 x %2").arg(img.width()).arg(img.height()));
    }

    if (m_recording) {
        const QString fn = QString("%1/frame_%2.jpg")
                               .arg(m_recDir)
                               .arg(m_recFrames++, 6, 10, QChar('0'));
        QFile f(fn);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(data);
            f.close();
        }
    }
}

void MainWindow::onError() {
    setConnectionState("ERROR: " + m_socket.errorString(), QColor(231, 76, 60));
}

void MainWindow::takeSnapshot() {
    const QImage img = m_video->currentFrame();
    if (img.isNull()) {
        statusBar()->showMessage("No frame available for snapshot", 3000);
        return;
    }
    QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/ESP32CAM";
    QDir().mkpath(dir);
    const QString fn = dir + "/snapshot_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
    if (img.save(fn, "JPG", 92)) {
        statusBar()->showMessage("Snapshot saved: " + fn, 5000);
    } else {
        statusBar()->showMessage("Failed to save snapshot", 5000);
    }
}

void MainWindow::toggleRecording(bool on) {
    if (on) {
        const QString base =
            QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/ESP32CAM";
        m_recDir = base + "/rec_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QDir().mkpath(m_recDir);
        m_recFrames = 0;
        m_recording = true;
        m_recTimer.restart();
        m_video->setRecording(true, 0);
        m_recordAct->setText("Stop");
        statusBar()->showMessage("Recording frames to: " + m_recDir, 4000);
    } else {
        m_recording = false;
        m_video->setRecording(false);
        m_recordAct->setText("Record");
        statusBar()->showMessage(
            QString("Recording stopped (%1 frames saved)").arg(m_recFrames), 6000);
    }
}

void MainWindow::toggleFullScreen(bool on) {
    if (on) {
        showFullScreen();
    } else {
        showNormal();
    }
}

void MainWindow::tickStats() {
    const double kbps = (m_bytesSinceTick * 8.0) / 1000.0;

    m_fpsLabel->setText(QString::number(m_frameCount));
    m_rateLabel->setText(kbps >= 1000.0
                             ? QString("%1 Mbps").arg(kbps / 1000.0, 0, 'f', 2)
                             : QString("%1 kbps").arg(kbps, 0, 'f', 0));
    m_framesLabel->setText(QString::number(m_totalFrames));

    m_video->setFps(m_frameCount);
    if (m_recording) m_video->setRecording(true, m_recTimer.elapsed());
    m_video->update();   // refresh clock / overlays even without new frames

    m_frameCount = 0;
    m_bytesSinceTick = 0;
}
