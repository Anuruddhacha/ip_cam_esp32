#pragma once

#include <QWidget>
#include <QImage>
#include <QString>

// Renders the live camera frame with industrial-style overlays:
// camera name, live timestamp, resolution/FPS, a REC indicator, and a
// "NO SIGNAL" placeholder when there is no video.
class VideoWidget : public QWidget {
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &img);
    void clearFrame();
    void setConnected(bool connected);
    void setCameraName(const QString &name);
    void setFps(double fps);
    void setRecording(bool recording, qint64 elapsedMs = 0);

    QImage currentFrame() const { return m_frame; }

signals:
    void doubleClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void drawOverlays(QPainter &p);
    void drawLabel(QPainter &p, const QRect &area, Qt::Alignment align,
                   const QString &text, const QColor &color);

    QImage  m_frame;
    bool    m_connected = false;
    bool    m_recording = false;
    qint64  m_recElapsedMs = 0;
    QString m_cameraName = "CAM 01";
    double  m_fps = 0.0;
};
