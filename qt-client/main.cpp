#include <QApplication>
#include "app/Theme.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("AXIO-CAM Viewer");
    QApplication::setOrganizationName("AXIO_CAM");

    Theme::applyDarkBlue(app);

    MainWindow window;
    window.show();
    return app.exec();
}
