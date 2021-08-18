#include "locationitem.h"
#include <QMetaType>
#include <QSharedPointer>

const int typeIdVectorModelLocationItem = qRegisterMetaType<QSharedPointer<QVector<locationsmodel::LocationItem> >>("QSharedPointer<QVector<locationsmodel::LocationItem> >");
const int typeIdModelLocationItem = qRegisterMetaType<locationsmodel::LocationItem>("locationsmodel::LocationItem");
