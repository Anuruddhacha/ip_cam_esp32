#include <QApplication>
#include "app/Theme.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("ESP32-CAM Industrial Viewer");
    QApplication::setOrganizationName("ESP32CAM");

    Theme::applyDarkBlue(app);

    MainWindow window;
    window.show();
    return app.exec();
}
