#pragma once

#include <QWidget>
#include <QString>
#include <QSize>
#include <QColor>

class QLineEdit;
class QCheckBox;
class QLabel;

// The side dock contents: connection settings plus a live stream-info readout.
// Knows nothing about networking - it just exposes inputs and display slots.
class ControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);

    QString url() const;
    QString cameraName() const;
    bool autoReconnect() const;

public slots:
    void setStats(int fps, double kbps, quint64 total);
    void setResolution(const QSize &size);
    void setConnectionState(const QString &text, const QColor &color);

signals:
    void cameraNameChanged(const QString &name);
    void urlSubmitted();

private:
    QLineEdit *m_url  = nullptr;
    QLineEdit *m_name = nullptr;
    QCheckBox *m_auto = nullptr;

    QLabel *m_status = nullptr;
    QLabel *m_res    = nullptr;
    QLabel *m_fps    = nullptr;
    QLabel *m_rate   = nullptr;
    QLabel *m_frames = nullptr;
};
