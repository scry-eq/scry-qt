#include "app/MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("scry-qt");
    app.setOrganizationName("scry-eq");

    MainWindow w;
    w.show();

    return app.exec();
}
