#include "app/Theme.h"

#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include <QString>

namespace Theme {

void applyDarkBlue(QApplication &app) {
    if (QStyleFactory::keys().contains("Fusion")) {
        app.setStyle("Fusion");
    }

    // Base palette (covers native controls the stylesheet doesn't reach).
    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(Color::Background));
    pal.setColor(QPalette::WindowText,      QColor(Color::Text));
    pal.setColor(QPalette::Base,            QColor(Color::Surface));
    pal.setColor(QPalette::AlternateBase,   QColor(Color::Panel));
    pal.setColor(QPalette::Text,            QColor(Color::Text));
    pal.setColor(QPalette::Button,          QColor(Color::Panel));
    pal.setColor(QPalette::ButtonText,      QColor(Color::Text));
    pal.setColor(QPalette::Highlight,       QColor(Color::Accent));
    pal.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    pal.setColor(QPalette::ToolTipBase,     QColor(Color::Surface));
    pal.setColor(QPalette::ToolTipText,     QColor(Color::Text));
    pal.setColor(QPalette::PlaceholderText, QColor(Color::TextMuted));
    pal.setColor(QPalette::Disabled, QPalette::Text, QColor(Color::TextMuted));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(Color::TextMuted));
    app.setPalette(pal);

    const QString qss = QString(R"(
        QMainWindow, QWidget { background: %(bg)s; color: %(text)s;
            font-size: 13px; }

        /* Toolbar */
        QToolBar { background: %(panel)s; border: 0; border-bottom: 1px solid %(border)s;
            padding: 6px; spacing: 6px; }
        QToolBar QToolButton { color: %(text)s; padding: 7px 12px; border-radius: 6px;
            font-weight: 600; }
        QToolBar QToolButton:hover { background: %(surface)s; }
        QToolBar QToolButton:pressed { background: %(accentdim)s; }
        QToolBar QToolButton:checked { background: %(accent)s; color: #ffffff; }
        QToolBar QToolButton:disabled { color: %(muted)s; }
        QToolBar::separator { background: %(border)s; width: 1px; margin: 4px 6px; }

        /* Dock / panels */
        QDockWidget { color: %(text)s; titlebar-close-icon: none; titlebar-normal-icon: none; }
        QDockWidget::title { background: %(panel)s; padding: 8px 10px;
            border-bottom: 1px solid %(border)s; font-weight: 700; }

        QGroupBox { background: %(panel)s; border: 1px solid %(border)s; border-radius: 8px;
            margin-top: 14px; padding: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 2px 6px;
            color: %(accent)s; font-weight: 700; }

        /* Inputs */
        QLineEdit { background: %(surface)s; border: 1px solid %(border)s; border-radius: 6px;
            padding: 7px 9px; color: %(text)s; selection-background-color: %(accent)s; }
        QLineEdit:focus { border: 1px solid %(accent)s; }

        QCheckBox { color: %(text)s; spacing: 8px; }
        QCheckBox::indicator { width: 16px; height: 16px; border-radius: 4px;
            border: 1px solid %(border)s; background: %(surface)s; }
        QCheckBox::indicator:checked { background: %(accent)s; border: 1px solid %(accent)s; }

        QLabel { color: %(text)s; }

        /* Status bar */
        QStatusBar { background: %(panel)s; color: %(muted)s;
            border-top: 1px solid %(border)s; }
        QStatusBar QLabel { padding: 0 10px; }
        QStatusBar::item { border: 0; }

        /* Scrollbars */
        QScrollBar:vertical { background: %(panel)s; width: 10px; margin: 0; }
        QScrollBar::handle:vertical { background: %(border)s; border-radius: 5px; min-height: 24px; }
        QScrollBar::handle:vertical:hover { background: %(accentdim)s; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
    )")
        .replace("%(bg)s",        Color::Background)
        .replace("%(panel)s",     Color::Panel)
        .replace("%(surface)s",   Color::Surface)
        .replace("%(border)s",    Color::Border)
        .replace("%(accent)s",    Color::Accent)
        .replace("%(accentdim)s", Color::AccentDim)
        .replace("%(text)s",      Color::Text)
        .replace("%(muted)s",     Color::TextMuted);

    app.setStyleSheet(qss);
}

}  // namespace Theme
