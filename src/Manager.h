#ifndef MANAGER_H
#define MANAGER_H

#include <QDir>

class Manager
{
public:
    static Manager &get()
    {
        static Manager manager;
        return manager;
    }

    QDir homeDir;

private:
    void loadConfig();

    Manager() noexcept
    {
        loadConfig();
    }
};

#endif // MANAGER_H
