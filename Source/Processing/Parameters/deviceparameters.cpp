#include "deviceparameters.h"

DeviceParameters::DeviceParameters(QObject *parent)
    : ParameterStore(DeviceParamDefs::defaultGroupMeta(),
                     DeviceParamDefs::defaultSubGroupMeta(),
                     DeviceParamDefs::defaultParams(),
                     parent)
{
}
