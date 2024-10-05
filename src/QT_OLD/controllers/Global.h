#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDir>

class Global
{
public:
    static Global &get()
    {
        static Global manager;
        return manager;
    }

private:
    void loadConfig();

    Global() noexcept { loadConfig(); }

};

#endif // GLOBAL_H
