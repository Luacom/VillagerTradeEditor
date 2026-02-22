#include "villagereditor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VillagerEditor w;
    w.show();
    return a.exec();
}
