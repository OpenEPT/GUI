#ifndef APPLICATIONPARAMDEFS_H
#define APPLICATIONPARAMDEFS_H

#include "parameterdefs.h"

namespace ApplicationParamDefs
{
    enum Group
    {
        WorkspaceConfig = 0,
        ServicesConfig = 1
    };

    enum SubGroup
    {
        Workspace = 0,
        StreamService = 1,
        StatusService = 2,
        EnergyPointService = 3
    };

    QList<Params::GroupMeta> defaultGroupMeta();
    QList<Params::SubGroupMeta> defaultSubGroupMeta();
    QList<Params::Param> defaultParams();
}

#endif // APPLICATIONPARAMDEFS_H
