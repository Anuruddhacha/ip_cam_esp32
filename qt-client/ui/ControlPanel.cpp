#include "ui/ControlPanel.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>

namespace {
constexpr const char *DEFAULT_URL =
    "wss://ipcamserver-bdf1an9v.b4a.run/view";
}

ControlPanel::ControlPanel(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    // ---- Connection ----
    auto *connGroup = new QGroupBox("Connection", this);
    auto *form = new QFormLayout(connGroup);
    form->setSpacing(8);

    m_name = new QLineEdit("CAM 01", connGroup);
    m_url  = new QLineEdit(DEFAULT_URL, connGroup);
    m_url->setPlaceholderText("wss://host/view");
    m_auto = new QCheckBox("Auto-reconnect", connGroup);
    m_auto->setChecked(true);

    form->addRow("Name", m_name);
    form->addRow("Relay URL", m_url);
    form->addRow("", m_auto);
    root->addWidget(connGroup);

    connect(m_name, &QLineEdit::textChanged, this, &ControlPanel::cameraNameChanged);
    connect(m_url, &QLineEdit::returnPressed, this, &ControlPanel::urlSubmitted);

    // ---- Stream info ----
    auto *infoGroup = new QGroupBox("Stream", this);
    auto *info = new QFormLayout(infoGroup);
    info->setSpacing(8);

    m_status = new QLabel("OFFLINE", infoGroup);
    m_res    = new QLabel("-", infoGroup);
    m_fps    = new QLabel("0", infoGroup);
    m_rate   = new QLabel("0 kbps", infoGroup);
    m_frames = new QLabel("0", infoGroup);

    info->addRow("Status", m_status);
    info->addRow("Resolution", m_res);
    info->addRow("FPS", m_fps);
    info->addRow("Bitrate", m_rate);
    info->addRow("Frames", m_frames);
    root->addWidget(infoGroup);

    root->addStretch(1);
}

QString ControlPanel::url() const { return m_url->text().trimmed(); }
QString ControlPanel::cameraName() const { return m_name->text(); }
bool ControlPanel::autoReconnect() const { return m_auto->isChecked(); }

void ControlPanel::setStats(int fps, double kbps, quint64 total) {
    m_fps->setText(QString::number(fps));
    m_rate->setText(kbps >= 1000.0
                        ? QString("%1 Mbps").arg(kbps / 1000.0, 0, 'f', 2)
                        : QString("%1 kbps").arg(kbps, 0, 'f', 0));
    m_frames->setText(QString::number(total));
}

void ControlPanel::setResolution(const QSize &size) {
    m_res->setText(QString("%1 x %2").arg(size.width()).arg(size.height()));
}

void ControlPanel::setConnectionState(const QString &text, const QColor &color) {
    m_status->setText(text);
    m_status->setStyleSheet(QString("color:%1; font-weight:700;").arg(color.name()));
}
