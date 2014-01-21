
#include <QApplication>
#include <NFrusPMainWindow.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    NFrusPMainWindow window;
    window.show();
    return app.exec();
}
