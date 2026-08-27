#include "applicationparameters.h"

ApplicationParameters::ApplicationParameters(QObject *parent)
    : ParameterStore(ApplicationParamDefs::defaultGroupMeta(),
                     ApplicationParamDefs::defaultSubGroupMeta(),
                     ApplicationParamDefs::defaultParams(),
                     parent)
{
}
