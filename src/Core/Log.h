#ifndef CZ_SHELFLOG_H
#define CZ_SHELFLOG_H

#include <CZ/Core/CZLogger.h>

#define Log ShelfLogger()

const CZ::CZLogger &ShelfLogger() noexcept;

#endif // CZ_SHELFLOG_H
