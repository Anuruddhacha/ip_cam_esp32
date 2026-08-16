#pragma once

#include <QMainWindow>
#include "core/StreamClient.h"

class VideoWidget;
class ControlPanel;
class StreamStats;
class FrameRecorder;
class QAction;

// Top-level window. Pure composition/wiring: it connects the StreamClient
// (network) to the VideoWidget (render), StreamStats (metrics), FrameRecorder
// (capture) and ControlPanel (settings/readout).
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onConnectTriggered();
    void onStreamState(StreamClient::State state, const QString &message);
    void onSnapshot();
    void onRecordToggled(bool on);
    void onFullScreenToggled(bool on);

private:
    void buildToolbar();
    void buildDock();
    void wireSignals();

    VideoWidget   *m_video    = nullptr;
    ControlPanel  *m_panel    = nullptr;
    StreamClient  *m_client   = nullptr;
    StreamStats   *m_stats    = nullptr;
    FrameRecorder *m_recorder = nullptr;

    QAction *m_connectAct    = nullptr;
    QAction *m_snapshotAct   = nullptr;
    QAction *m_recordAct     = nullptr;
    QAction *m_fullscreenAct = nullptr;
};
