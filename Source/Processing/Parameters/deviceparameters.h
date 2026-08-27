#ifndef DEVICEPARAMETERS_H
#define DEVICEPARAMETERS_H

#include "parameterstore.h"
#include "deviceparamdefs.h"

class DeviceParameters : public ParameterStore
{
    Q_OBJECT

public:
    explicit DeviceParameters(QObject *parent = nullptr);
};

#endif // DEVICEPARAMETERS_H
