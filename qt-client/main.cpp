#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("ESP32-CAM Industrial Viewer");
    QApplication::setOrganizationName("ESP32CAM");

    // Consistent cross-platform base style; the dark theme is applied as a
    // stylesheet in MainWindow.
    if (QStyleFactory::keys().contains("Fusion")) {
        QApplication::setStyle("Fusion");
    }

    MainWindow window;
    window.show();
    return app.exec();
}
