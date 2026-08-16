#pragma once

#include <QWidget>
#include <QImage>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>

// Renders the live frame with industrial overlays (camera name, timestamp,
// resolution/FPS, REC indicator) and a NO-SIGNAL placeholder. Self-refreshes
// so the clock and REC blink update even without new frames.
class VideoWidget : public QWidget {
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &img);
    void clear();
    void setConnected(bool connected);
    void setCameraName(const QString &name);
    void setFps(int fps);
    void setRecording(bool recording);

    QImage currentFrame() const { return m_frame; }

signals:
    void doubleClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void drawOverlays(QPainter &p);
    void drawBadge(QPainter &p, const QRect &area, Qt::Alignment align,
                   const QString &text, const QColor &color);

    QImage  m_frame;
    bool    m_connected = false;
    bool    m_recording = false;
    QString m_cameraName = "CAM 01";
    int     m_fps = 0;

    QTimer        m_refresh;   // drives overlay clock / blink
    QElapsedTimer m_recTimer;
};
