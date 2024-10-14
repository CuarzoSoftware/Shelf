#include <QSocketNotifier>
#include <QApplication>
#include <core/Shelf.h>

int main(int argc, char **argv)
{
    QApplication app { argc, argv };

    const int fd { Shelf::fd() };

    if (fd < 0)
        return EXIT_FAILURE;

    QSocketNotifier sockRead { fd, QSocketNotifier::Read  };

    QObject::connect(&sockRead, &QSocketNotifier::activated, []() {

        Shelf::dispatch();
        Shelf::flush();
    });

    sockRead.setEnabled(true);

    return app.exec();
}
