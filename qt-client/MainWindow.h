#pragma once

#include <QMainWindow>
#include <QtWebSockets/QWebSocket>
#include <QElapsedTimer>
#include <QSize>

class VideoWidget;
class QLineEdit;
class QCheckBox;
class QAction;
class QLabel;

// Industrial-style desktop viewer for the ESP32-CAM relay.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void toggleConnection();
    void onConnected();
    void onDisconnected();
    void onBinaryMessage(const QByteArray &data);
    void onError();

    void takeSnapshot();
    void toggleRecording(bool on);
    void toggleFullScreen(bool on);
    void tickStats();   // 1 Hz: refresh FPS / bitrate / clock overlay

private:
    void buildUi();
    void buildToolbar();
    void buildDock();
    void buildStatusBar();
    void applyTheme();
    void setConnectionState(const QString &text, const QColor &color);
    void openConnection();

    VideoWidget *m_video = nullptr;

    // Connection panel
    QLineEdit *m_urlEdit       = nullptr;
    QLineEdit *m_nameEdit      = nullptr;
    QCheckBox *m_autoReconnect = nullptr;

    // Toolbar actions
    QAction *m_connectAct    = nullptr;
    QAction *m_snapshotAct   = nullptr;
    QAction *m_recordAct     = nullptr;
    QAction *m_fullscreenAct = nullptr;

    // Status bar widgets
    QLabel *m_statusText  = nullptr;
    QLabel *m_resLabel    = nullptr;
    QLabel *m_fpsLabel    = nullptr;
    QLabel *m_rateLabel   = nullptr;
    QLabel *m_framesLabel = nullptr;

    QWebSocket m_socket;
    bool m_wantConnected = false;

    // Stats
    int     m_frameCount = 0;       // frames since last tick
    quint64 m_totalFrames = 0;
    quint64 m_bytesSinceTick = 0;
    QSize   m_lastSize;

    // Recording (saves incoming JPEG frames to a session folder)
    bool    m_recording = false;
    QString m_recDir;
    quint64 m_recFrames = 0;
    QElapsedTimer m_recTimer;
};
