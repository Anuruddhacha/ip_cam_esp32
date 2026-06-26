#include "VideoWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDateTime>
#include <QFontMetrics>
#include <QStyle>

VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(480, 360);
    setAttribute(Qt::WA_OpaquePaintEvent);
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSizePolicy(sp);
}

void VideoWidget::setFrame(const QImage &img) {
    m_frame = img;
    update();
}

void VideoWidget::clearFrame() {
    m_frame = QImage();
    update();
}

void VideoWidget::setConnected(bool connected) {
    m_connected = connected;
    if (!connected) m_frame = QImage();
    update();
}

void VideoWidget::setCameraName(const QString &name) {
    m_cameraName = name;
    update();
}

void VideoWidget::setFps(double fps) {
    m_fps = fps;
}

void VideoWidget::setRecording(bool recording, qint64 elapsedMs) {
    m_recording = recording;
    m_recElapsedMs = elapsedMs;
    update();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    emit doubleClicked();
}

void VideoWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Background
    p.fillRect(rect(), QColor(12, 13, 16));

    if (!m_frame.isNull()) {
        QSize scaled = m_frame.size();
        scaled.scale(size(), Qt::KeepAspectRatio);
        QRect target(QPoint(0, 0), scaled);
        target.moveCenter(rect().center());
        p.drawImage(target, m_frame);

        // Thin frame around the image
        p.setPen(QPen(QColor(40, 42, 48), 1));
        p.drawRect(target.adjusted(0, 0, -1, -1));
    } else {
        // NO SIGNAL placeholder with crosshair
        p.setPen(QPen(QColor(45, 47, 54), 1, Qt::DashLine));
        p.drawLine(rect().left(), rect().center().y(), rect().right(), rect().center().y());
        p.drawLine(rect().center().x(), rect().top(), rect().center().x(), rect().bottom());

        QFont f = p.font();
        f.setPointSize(18);
        f.setBold(true);
        p.setFont(f);
        p.setPen(m_connected ? QColor(230, 170, 0) : QColor(120, 120, 128));
        p.drawText(rect(), Qt::AlignCenter,
                   m_connected ? "WAITING FOR SIGNAL" : "NO SIGNAL");
    }

    drawOverlays(p);
}

void VideoWidget::drawLabel(QPainter &p, const QRect &area, Qt::Alignment align,
                            const QString &text, const QColor &color) {
    QFontMetrics fm(p.font());
    QRect tb = fm.boundingRect(text);
    tb.adjust(-8, -4, 8, 4);

    QRect placed = QStyle::alignedRect(Qt::LeftToRight, align, tb.size(), area);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 130));
    p.drawRoundedRect(placed, 4, 4);

    p.setPen(color);
    p.drawText(placed, Qt::AlignCenter, text);
}

void VideoWidget::drawOverlays(QPainter &p) {
    const QRect area = rect().adjusted(10, 10, -10, -10);

    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);

    // Top-left: camera name
    drawLabel(p, area, Qt::AlignTop | Qt::AlignLeft, m_cameraName, QColor(230, 230, 235));

    // Top-right: live timestamp
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss");
    drawLabel(p, area, Qt::AlignTop | Qt::AlignRight, ts, QColor(210, 210, 215));

    // Bottom-left: resolution + fps
    if (!m_frame.isNull()) {
        const QString info = QString("%1x%2   %3 FPS")
                                 .arg(m_frame.width())
                                 .arg(m_frame.height())
                                 .arg(m_fps, 0, 'f', 0);
        drawLabel(p, area, Qt::AlignBottom | Qt::AlignLeft, info, QColor(120, 220, 140));
    }

    // Bottom-right / center-top: REC indicator
    if (m_recording) {
        const qint64 s = m_recElapsedMs / 1000;
        const QString rec = QString("REC  %1:%2:%3")
                                .arg(s / 3600, 2, 10, QChar('0'))
                                .arg((s % 3600) / 60, 2, 10, QChar('0'))
                                .arg(s % 60, 2, 10, QChar('0'));

        QFontMetrics fm(p.font());
        QRect tb = fm.boundingRect(rec);
        tb.adjust(-26, -4, 8, 4);
        QRect placed = QStyle::alignedRect(Qt::LeftToRight,
                                           Qt::AlignTop | Qt::AlignHCenter,
                                           tb.size(), area);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 140));
        p.drawRoundedRect(placed, 4, 4);

        // blinking dot (on for even seconds)
        if ((m_recElapsedMs / 500) % 2 == 0) {
            p.setBrush(QColor(230, 40, 40));
            p.drawEllipse(QPoint(placed.left() + 12, placed.center().y()), 5, 5);
        }
        p.setPen(QColor(240, 120, 120));
        p.drawText(placed.adjusted(24, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, rec);
    }
}
