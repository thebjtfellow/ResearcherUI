

#include "mainwindow.h"

#include <QApplication>
#include <QVariant>
#include <QList>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QList<float_t>>("QList<float_t>");
    qRegisterMetaType<QCheckBox*>("QCheckBox*");
    QCoreApplication::setOrganizationName("b-jetting");
    QCoreApplication::setOrganizationDomain("b-jetting.com");
    QCoreApplication::setApplicationName("stl-slicer");

    MainWindow w;
    w.show();
    return a.exec();
}
