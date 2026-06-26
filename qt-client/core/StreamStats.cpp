#include "core/StreamStats.h"

StreamStats::StreamStats(QObject *parent) : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &StreamStats::tick);
    m_timer.start(1000);
}

void StreamStats::addFrame(int bytes) {
    ++m_framesThisSecond;
    ++m_total;
    m_bytesThisSecond += static_cast<quint64>(bytes);
}

void StreamStats::reset() {
    m_framesThisSecond = 0;
    m_bytesThisSecond = 0;
    m_total = 0;
}

void StreamStats::tick() {
    const double kbps = (m_bytesThisSecond * 8.0) / 1000.0;
    emit updated(m_framesThisSecond, kbps, m_total);
    m_framesThisSecond = 0;
    m_bytesThisSecond = 0;
}
