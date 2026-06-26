#include "core/FrameRecorder.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>

FrameRecorder::FrameRecorder(QObject *parent) : QObject(parent) {}

qint64 FrameRecorder::elapsedMs() const {
    return m_recording ? m_timer.elapsed() : 0;
}

bool FrameRecorder::start() {
    if (m_recording) return true;

    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/ESP32CAM";
    m_dir = base + "/rec_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    if (!QDir().mkpath(m_dir)) {
        return false;
    }

    m_count = 0;
    m_recording = true;
    m_timer.restart();
    emit started(m_dir);
    return true;
}

void FrameRecorder::stop() {
    if (!m_recording) return;
    m_recording = false;
    emit stopped(m_count);
}

void FrameRecorder::writeFrame(const QByteArray &jpeg) {
    if (!m_recording) return;

    const QString fn = QString("%1/frame_%2.jpg")
                           .arg(m_dir)
                           .arg(m_count, 6, 10, QChar('0'));
    QFile f(fn);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(jpeg);
        f.close();
        ++m_count;
    }
}
