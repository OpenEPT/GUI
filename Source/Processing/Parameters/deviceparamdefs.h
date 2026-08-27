#ifndef DEVICEPARAMDEFS_H
#define DEVICEPARAMDEFS_H

#include "parameterdefs.h"

namespace DeviceParamDefs
{
    enum Group
    {
        DeviceConfig = 0,
        ApplicationConfig = 1,
        RuntimeState = 2,
        Calculated = 3,
        ChargerConfig = 4
    };

    enum SubGroup
    {
        General = 0,
        Network = 1,
        Stream = 2,
        ADC = 3,
        EnergyPoint = 4,
        Processing = 5,
        Calibration = 6,
        Load = 7,
        ChargerGeneral = 8,
        Battery = 9,
        Protection = 10,
        Statistics = 11,
        FileStorage = 12,
        ChargerConf = 13,
        ChargerBD = 14
    };

    QList<Params::GroupMeta> defaultGroupMeta();
    QList<Params::SubGroupMeta> defaultSubGroupMeta();
    QList<Params::Param> defaultParams();
}

#endif // DEVICEPARAMDEFS_H
