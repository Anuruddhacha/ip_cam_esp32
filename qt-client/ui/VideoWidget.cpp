#include "ui/VideoWidget.h"
#include "app/Theme.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDateTime>
#include <QFontMetrics>
#include <QStyle>

VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(480, 360);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Refresh overlays (clock, REC blink) ~2x/sec without new frames.
    connect(&m_refresh, &QTimer::timeout, this, [this]() { update(); });
    m_refresh.start(500);
}

void VideoWidget::setFrame(const QImage &img) {
    m_frame = img;
    update();
}

void VideoWidget::clear() {
    m_frame = QImage();
    update();
}

void VideoWidget::setConnected(bool connected) {
    m_connected = connected;
    if (!connected) m_frame = QImage();
    update();
}

void VideoWidget::setCameraName(const QString &name) {
    m_cameraName = name.isEmpty() ? QStringLiteral("CAM") : name;
    update();
}

void VideoWidget::setFps(int fps) {
    m_fps = fps;
}

void VideoWidget::setRecording(bool recording) {
    if (recording && !m_recording) m_recTimer.restart();
    m_recording = recording;
    update();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *) {
    emit doubleClicked();
}

void VideoWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    p.fillRect(rect(), QColor(Theme::Color::Background));

    if (!m_frame.isNull()) {
        QSize scaled = m_frame.size();
        scaled.scale(size(), Qt::KeepAspectRatio);
        QRect target(QPoint(0, 0), scaled);
        target.moveCenter(rect().center());
        p.drawImage(target, m_frame);

        p.setPen(QPen(QColor(Theme::Color::Border), 1));
        p.drawRect(target.adjusted(0, 0, -1, -1));
    } else {
        p.setPen(QPen(QColor(Theme::Color::Border), 1, Qt::DashLine));
        p.drawLine(rect().left(), rect().center().y(), rect().right(), rect().center().y());
        p.drawLine(rect().center().x(), rect().top(), rect().center().x(), rect().bottom());

        QFont f = p.font();
        f.setPointSize(18);
        f.setBold(true);
        p.setFont(f);
        p.setPen(m_connected ? QColor(Theme::Color::Accent) : QColor(Theme::Color::TextMuted));
        p.drawText(rect(), Qt::AlignCenter,
                   m_connected ? "WAITING FOR SIGNAL" : "NO SIGNAL");
    }

    drawOverlays(p);
}

void VideoWidget::drawBadge(QPainter &p, const QRect &area, Qt::Alignment align,
                            const QString &text, const QColor &color) {
    QFontMetrics fm(p.font());
    QRect tb = fm.boundingRect(text);
    tb.adjust(-9, -5, 9, 5);

    QRect placed = QStyle::alignedRect(Qt::LeftToRight, align, tb.size(), area);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 140));
    p.drawRoundedRect(placed, 5, 5);

    // subtle blue underline accent
    p.setPen(QPen(QColor(Theme::Color::Accent), 2));
    p.drawLine(placed.left() + 5, placed.bottom() - 1, placed.right() - 5, placed.bottom() - 1);

    p.setPen(color);
    p.drawText(placed, Qt::AlignCenter, text);
}

void VideoWidget::drawOverlays(QPainter &p) {
    const QRect area = rect().adjusted(12, 12, -12, -12);

    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);

    drawBadge(p, area, Qt::AlignTop | Qt::AlignLeft, m_cameraName,
              QColor(Theme::Color::Text));

    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd  HH:mm:ss");
    drawBadge(p, area, Qt::AlignTop | Qt::AlignRight, ts, QColor(Theme::Color::Text));

    if (!m_frame.isNull()) {
        const QString info = QString("%1 x %2    %3 FPS")
                                 .arg(m_frame.width())
                                 .arg(m_frame.height())
                                 .arg(m_fps);
        drawBadge(p, area, Qt::AlignBottom | Qt::AlignLeft, info,
                  QColor(Theme::Color::Accent));
    }

    if (m_recording) {
        const qint64 s = m_recTimer.elapsed() / 1000;
        const QString rec = QString("REC  %1:%2:%3")
                                .arg(s / 3600, 2, 10, QChar('0'))
                                .arg((s % 3600) / 60, 2, 10, QChar('0'))
                                .arg(s % 60, 2, 10, QChar('0'));

        QFontMetrics fm(p.font());
        QRect tb = fm.boundingRect(rec);
        tb.adjust(-30, -5, 9, 5);
        QRect placed = QStyle::alignedRect(Qt::LeftToRight,
                                           Qt::AlignTop | Qt::AlignHCenter,
                                           tb.size(), area);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 150));
        p.drawRoundedRect(placed, 5, 5);

        if ((m_recTimer.elapsed() / 500) % 2 == 0) {
            p.setBrush(QColor(Theme::Color::Rec));
            p.drawEllipse(QPoint(placed.left() + 14, placed.center().y()), 5, 5);
        }
        p.setPen(QColor(Theme::Color::Rec));
        p.drawText(placed.adjusted(26, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, rec);
    }
}
