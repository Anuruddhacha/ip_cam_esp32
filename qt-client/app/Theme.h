#pragma once

class QApplication;

// Centralized application styling. A single dark "black & blue" theme so the
// look is defined in one place rather than scattered across widgets.
namespace Theme {

// Accent / palette constants reused by custom-painted widgets (e.g. overlays).
namespace Color {
constexpr const char *Background = "#0a0d12";  // near-black app background
constexpr const char *Panel      = "#11151c";  // panels / docks
constexpr const char *Surface    = "#161b24";  // inputs / raised surfaces
constexpr const char *Border     = "#1e2633";
constexpr const char *Accent     = "#2f81f7";  // primary blue
constexpr const char *AccentDim  = "#1f6feb";
constexpr const char *Text       = "#dbe2ea";
constexpr const char *TextMuted  = "#8b95a4";
constexpr const char *Ok         = "#2ecc71";
constexpr const char *Warn       = "#e6a700";
constexpr const char *Error      = "#ff5252";
constexpr const char *Rec        = "#ff3b3b";
}

void applyDarkBlue(QApplication &app);

}  // namespace Theme
