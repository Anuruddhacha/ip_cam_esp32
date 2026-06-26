#pragma once

#include <QObject>
#include <QTimer>

// Accumulates per-frame counters and emits aggregated statistics once per
// second (frames-per-second, bitrate, and a running total).
class StreamStats : public QObject {
    Q_OBJECT

public:
    explicit StreamStats(QObject *parent = nullptr);

    quint64 totalFrames() const { return m_total; }

public slots:
    void addFrame(int bytes);
    void reset();

signals:
    // fps: frames in the last second, kbps: kilobits/s, total: lifetime frames
    void updated(int fps, double kbps, quint64 total);

private slots:
    void tick();

private:
    QTimer  m_timer;
    int     m_framesThisSecond = 0;
    quint64 m_bytesThisSecond = 0;
    quint64 m_total = 0;
};
