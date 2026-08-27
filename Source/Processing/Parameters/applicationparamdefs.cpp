#include "applicationparamdefs.h"

namespace ApplicationParamDefs
{

QList<Params::GroupMeta> defaultGroupMeta()
{
    return {
        {
            WorkspaceConfig,
            "Workspace Configuration",
            "Global application workspace parameters.",
            0
        },
        {
            ServicesConfig,
            "Services Configuration",
            "Base ports and global service settings used by the application.",
            1
        }
    };
}

QList<Params::SubGroupMeta> defaultSubGroupMeta()
{
    return {
        {
            Workspace,
            "Workspace",
            "Application workspace directory.",
            0
        },
        {
            StreamService,
            "Stream Service",
            "Global stream service parameters.",
            1
        },
        {
            StatusService,
            "Status Service",
            "Global status service parameters.",
            2
        },
        {
            EnergyPointService,
            "Energy Point Service",
            "Global energy point service parameters.",
            3
        }
    };
}

QList<Params::Param> defaultParams()
{
    return {
        {
            {
                "workspacePath",
                "Workspace Directory",
                "Root workspace directory selected during application startup.",
                "",
                WorkspaceConfig,
                Workspace,
                Params::Access::ReadOnly,
                Params::Storage::LoadSave,
                Params::Target::Application,
                "",
                QVariant(),
                QVariant(),
                {},
                true,
                0
            },
            "",
            true
        },
        {
            {
                "streamServiceBasePort",
                "Stream Service Base Port",
                "Base TCP/UDP port used for stream services.",
                "",
                ServicesConfig,
                StreamService,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Application,
                11223,
                0,
                65535,
                {},
                true,
                0
            },
            6000,
            true
        },
        {
            {
                "statusServiceBasePort",
                "Status Service Base Port",
                "Base TCP/UDP port used for status services.",
                "",
                ServicesConfig,
                StatusService,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Application,
                8818,
                0,
                65535,
                {},
                true,
                0
            },
            9000,
            true
        },
        {
            {
                "epServiceBasePort",
                "EP Service Base Port",
                "Base TCP/UDP port used for Energy Point services.",
                "",
                ServicesConfig,
                EnergyPointService,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Application,
                8000,
                0,
                65535,
                {},
                true,
                0
            },
            8000,
            true
        }
    };
}

}
