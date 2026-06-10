#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Register all bundled fonts before MainWindow loads
    QFontDatabase::addApplicationFont(":/resources/fonts/CourierPrime-Regular.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/CourierPrime-Bold.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/LibertinusMono-Regular.otf");
    QFontDatabase::addApplicationFont(":/resources/fonts/RedHatMono-Regular.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/RedHatMono-Bold.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/CascadiaMono-Regular.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/CascadiaMono-Bold.ttf");

    MainWindow w;

    // MainWindow handles its own size restorations!
    w.show();
    return a.exec();
}