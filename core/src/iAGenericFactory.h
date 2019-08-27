#include "open_iA_Core_export.h"

#include <QSharedPointer>

//! Generic factory class.
template <typename BaseType>
class open_iA_Core_API iAGenericFactory
{
public:
    virtual QSharedPointer<BaseType> create() = 0;
    virtual ~iAGenericFactory() {}
};

//! Factory for a specific typed derived from BaseType.
template <typename DerivedType, typename BaseType>
class open_iA_Core_API iASpecificFactory: public iAGenericFactory<BaseType>
{
public:
    QSharedPointer<BaseType> create() override
    {
        return DerivedType::create();
    }
};
