#include <Core/Log.h>

using namespace CZ;

const CZ::CZLogger &ShelfLogger() noexcept
{
    static CZLogger logger { "Shelf", "CZ_SHELF_LOG_LEVEL" };
    return logger;
}
