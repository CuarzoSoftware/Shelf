#ifndef ITEMMODEL_H
#define ITEMMODEL_H

#include <QString>

class ItemModel
{
public:
    ItemModel();
    virtual ~ItemModel() = default;
    virtual const QString &name() const noexcept = 0;
};

#endif // ITEMMODEL_H
