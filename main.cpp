#include "cristall.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Cristall w;
    w.show();
    return a.exec();
}
