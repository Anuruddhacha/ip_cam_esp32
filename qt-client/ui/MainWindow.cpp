#include "ui/MainWindow.h"
#include "ui/VideoWidget.h"
#include "ui/ControlPanel.h"
#include "core/StreamClient.h"
#include "core/StreamStats.h"
#include "core/FrameRecorder.h"
#include "app/Theme.h"

#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QAction>
#include <QStyle>
#include <QKeySequence>
#include <QImage>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_client   = new StreamClient(this);
    m_stats    = new StreamStats(this);
    m_recorder = new FrameRecorder(this);

    m_video = new VideoWidget(this);
    setCentralWidget(m_video);

    buildToolbar();
    buildDock();
    statusBar()->showMessage("Ready");
    wireSignals();

    setWindowTitle("AXIO-CAM Viewer");
    resize(1180, 760);

    m_panel->setConnectionState("OFFLINE", QColor(Theme::Color::Error));
    m_video->setCameraName(m_panel->cameraName());
}

void MainWindow::buildToolbar() {
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    const QStyle *st = style();

    m_connectAct = tb->addAction(st->standardIcon(QStyle::SP_MediaPlay), "Connect");
    m_connectAct->setShortcut(QKeySequence("Ctrl+K"));
    connect(m_connectAct, &QAction::triggered, this, &MainWindow::onConnectTriggered);

    tb->addSeparator();

    m_snapshotAct = tb->addAction(st->standardIcon(QStyle::SP_DialogSaveButton), "Snapshot");
    m_snapshotAct->setShortcut(QKeySequence("Ctrl+S"));
    m_snapshotAct->setEnabled(false);
    connect(m_snapshotAct, &QAction::triggered, this, &MainWindow::onSnapshot);

    m_recordAct = tb->addAction(st->standardIcon(QStyle::SP_DialogYesButton), "Record");
    m_recordAct->setCheckable(true);
    m_recordAct->setShortcut(QKeySequence("Ctrl+R"));
    m_recordAct->setEnabled(false);
    connect(m_recordAct, &QAction::toggled, this, &MainWindow::onRecordToggled);

    tb->addSeparator();

    m_fullscreenAct = tb->addAction(st->standardIcon(QStyle::SP_TitleBarMaxButton), "Fullscreen");
    m_fullscreenAct->setCheckable(true);
    m_fullscreenAct->setShortcut(QKeySequence("F11"));
    connect(m_fullscreenAct, &QAction::toggled, this, &MainWindow::onFullScreenToggled);
}

void MainWindow::buildDock() {
    auto *dock = new QDockWidget("Camera", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_panel = new ControlPanel(dock);
    dock->setWidget(m_panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::wireSignals() {
    // Network -> render / stats / recorder
    connect(m_client, &StreamClient::frameReceived, this,
            [this](const QByteArray &jpeg, const QImage &image) {
                m_video->setFrame(image);
                m_stats->addFrame(jpeg.size());
                m_panel->setResolution(image.size());
                m_recorder->writeFrame(jpeg);
            });

    connect(m_client, &StreamClient::stateChanged, this, &MainWindow::onStreamState);
    connect(m_client, &StreamClient::errorOccurred, this, [this](const QString &e) {
        m_panel->setConnectionState("ERROR: " + e, QColor(Theme::Color::Error));
        statusBar()->showMessage("Error: " + e, 5000);
    });

    // Stats -> panel + overlay
    connect(m_stats, &StreamStats::updated, this,
            [this](int fps, double kbps, quint64 total) {
                m_panel->setStats(fps, kbps, total);
                m_video->setFps(fps);
            });

    // Panel -> camera name / submit-to-connect
    connect(m_panel, &ControlPanel::cameraNameChanged, m_video, &VideoWidget::setCameraName);
    connect(m_panel, &ControlPanel::urlSubmitted, this, &MainWindow::onConnectTriggered);

    // Recorder feedback
    connect(m_recorder, &FrameRecorder::started, this, [this](const QString &dir) {
        statusBar()->showMessage("Recording to: " + dir, 4000);
    });
    connect(m_recorder, &FrameRecorder::stopped, this, [this](quint64 frames) {
        statusBar()->showMessage(QString("Recording stopped (%1 frames)").arg(frames), 6000);
    });

    // Video double-click -> fullscreen
    connect(m_video, &VideoWidget::doubleClicked, this, [this]() {
        m_fullscreenAct->toggle();
    });
}

void MainWindow::onConnectTriggered() {
    if (m_client->isActive()) {
        m_client->stop();
    } else {
        m_client->setUrl(m_panel->url());
        m_client->setAutoReconnect(m_panel->autoReconnect());
        m_video->setCameraName(m_panel->cameraName());
        m_client->start();
    }
}

void MainWindow::onStreamState(StreamClient::State state, const QString &message) {
    QColor color(Theme::Color::TextMuted);
    switch (state) {
    case StreamClient::State::Connected:
        color = QColor(Theme::Color::Ok);
        m_connectAct->setText("Disconnect");
        m_connectAct->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
        m_snapshotAct->setEnabled(true);
        m_recordAct->setEnabled(true);
        m_video->setConnected(true);
        break;
    case StreamClient::State::Connecting:
    case StreamClient::State::Reconnecting:
        color = QColor(Theme::Color::Warn);
        m_video->setConnected(true);   // shows "WAITING FOR SIGNAL"
        break;
    case StreamClient::State::Disconnected:
        color = QColor(Theme::Color::Error);
        m_connectAct->setText("Connect");
        m_connectAct->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        m_snapshotAct->setEnabled(false);
        if (m_recordAct->isChecked()) m_recordAct->setChecked(false);
        m_recordAct->setEnabled(false);
        m_video->setConnected(false);
        break;
    }
    m_panel->setConnectionState(message, color);
    statusBar()->showMessage(message);
}

void MainWindow::onSnapshot() {
    const QImage img = m_video->currentFrame();
    if (img.isNull()) {
        statusBar()->showMessage("No frame available for snapshot", 3000);
        return;
    }
    QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/ESP32CAM";
    QDir().mkpath(dir);
    const QString fn = dir + "/snapshot_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
    statusBar()->showMessage(img.save(fn, "JPG", 92) ? "Snapshot saved: " + fn
                                                     : "Failed to save snapshot", 5000);
}

void MainWindow::onRecordToggled(bool on) {
    if (on) {
        if (!m_recorder->start()) {
            statusBar()->showMessage("Failed to start recording", 4000);
            m_recordAct->setChecked(false);
            return;
        }
        m_recordAct->setText("Stop");
        m_video->setRecording(true);
    } else {
        m_recorder->stop();
        m_recordAct->setText("Record");
        m_video->setRecording(false);
    }
}

void MainWindow::onFullScreenToggled(bool on) {
    if (on) showFullScreen();
    else showNormal();
}
