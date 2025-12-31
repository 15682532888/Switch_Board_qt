#include "switch_board.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Switch_Board w;
    w.show();
    return a.exec();
}
