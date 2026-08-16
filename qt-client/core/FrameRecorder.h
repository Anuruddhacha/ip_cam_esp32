#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QElapsedTimer>

// Records incoming JPEG frames to a timestamped session folder on disk.
// (Frame-sequence capture; assemble to video later with ffmpeg if desired.)
class FrameRecorder : public QObject {
    Q_OBJECT

public:
    explicit FrameRecorder(QObject *parent = nullptr);

    bool    isRecording() const { return m_recording; }
    qint64  elapsedMs() const;
    quint64 frameCount() const { return m_count; }
    QString sessionDir() const { return m_dir; }

public slots:
    bool start();   // returns false if the session folder can't be created
    void stop();
    void writeFrame(const QByteArray &jpeg);

signals:
    void started(const QString &dir);
    void stopped(quint64 frames);

private:
    bool          m_recording = false;
    QString       m_dir;
    quint64       m_count = 0;
    QElapsedTimer m_timer;
};
