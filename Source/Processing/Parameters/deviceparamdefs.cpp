#include "deviceparamdefs.h"

namespace DeviceParamDefs
{

QList<Params::GroupMeta> defaultGroupMeta()
{
    return {
        {
            DeviceParamDefs::DeviceConfig,
            "Device Configuration",
            "Parameters stored on the device or directly related to device setup.",
            0
        },
        {
            DeviceParamDefs::RuntimeState,
            "Runtime State",
            "Dynamic parameters updated during execution.",
            2
        },
        {
            DeviceParamDefs::Calculated,
            "Calculated Values",
            "Values calculated by the application.",
            3
        },
        {
            DeviceParamDefs::ChargerConfig,
            "Charger Configuration",
            "Charger COnfiguration if it is connected",
            4
        }
    };
}

QList<Params::SubGroupMeta> defaultSubGroupMeta()
{
    return {
        {
            DeviceParamDefs::General,
            "General",
            "General device parameters.",
            0
        },
        {
            DeviceParamDefs::Network,
            "Network",
            "IP addresses, ports and communication endpoints.",
            1
        },
        {
            DeviceParamDefs::Stream,
            "Stream",
            "Streaming service parameters.",
            2
        },
        {
            DeviceParamDefs::ADC,
            "ADC",
            "ADC-related configuration and detected hardware parameters.",
            3
        },
        {
            DeviceParamDefs::EnergyPoint,
            "Energy Point",
            "Energy point processing and reporting parameters.",
            4
        },
        {
            DeviceParamDefs::Processing,
            "Processing",
            "Application-side data processing parameters.",
            5
        },
        {
            DeviceParamDefs::Calibration,
            "Calibration",
            "Application-side parameters used for calibration",
            6
        },
        {
            DeviceParamDefs::Load,
            "Load",
            "Load control and load-related runtime values.",
            7
        },
        {
            DeviceParamDefs::ChargerGeneral,
            "General Info",
            "General Charger Info",
            8
        },
        {
            DeviceParamDefs::Battery,
            "Battery",
            "Battery path and battery-related runtime values.",
            9
        },
        {
            DeviceParamDefs::Protection,
            "Protection",
            "Protection and fault indication states.",
            10
        },
        {
            DeviceParamDefs::Statistics,
            "Statistics",
            "Runtime statistics and monitoring values.",
            11
        },
        {
            DeviceParamDefs::FileStorage,
            "File Storage",
            "Application-side file saving parameters.",
            12
        },
        {
            DeviceParamDefs::ChargerConf,
            "Configuration",
            "Configurable Charger Parameters",
            13
        },
        {
            DeviceParamDefs::ChargerBD,
            "Memory",
            "Charger Memory Editing",
            14
        },
    };
}

QList<Params::Param> defaultParams()
{
    return {
        {
            {
                "deviceName",
                "Device Name",
                "User-defined device name.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::General,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                "",
                QVariant(),
                QVariant(),
                {},
                true,
                0
            },
            "",
            false
        },
        {
            {
                "deviceSerial",
                "Serial Number",
                "Device Serial Number",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::General,
                Params::Access::ReadOnly,
                Params::Storage::LoadSave,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                1
            },
            "",
            false
        },
        {
            {
                "fwVersion",
                "Firmware Version",
                "Device Firmware Version",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::General,
                Params::Access::ReadOnly,
                Params::Storage::LoadSave,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                2
            },
            "",
            false
        },
        {
            {
                "deviceMac",
                "Device MAC Address",
                "Device MAC Address",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Network,
                Params::Access::ReadOnly,
                Params::Storage::LoadSave,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                0
            },
            "",
            false
        },
        {
            {
                "deviceIp",
                "Device IP",
                "IP address used for communication with the device.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Network,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                "192.168.1.100",
                QVariant(),
                QVariant(),
                {},
                true,
                1
            },
            "192.168.1.100",
            false
        },
        {
            {
                "controlPort",
                "Control Port",
                "TCP port used for the control link.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Network,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                0,
                0,
                65535,
                {},
                true,
                2
            },
            0,
            false
        },
        {
            {
                "statusLinkPort",
                "Status Link Port",
                "Local port used for receiving device status messages.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Network,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                8818,
                0,
                65535,
                {},
                true,
                3
            },
            0,
            false
        },
        {
            {
                "energyPointLinkPort",
                "Energy Point Link Port",
                "Local port used for energy point communication.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Network,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                8000,
                0,
                65535,
                {},
                true,
                4
            },
            0,
            false
        },
        {
            {
                "streamLinkPort",
                "Stream Link Port",
                "Local port used for stream data reception.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Stream,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                11223,
                0,
                65535,
                {},
                true,
                5
            },
            0,
            false
        },
        {
            {
                "streamPacketSize",
                "Stream Packet Size",
                "Number of samples (per channel) expected in one stream packet.",
                "Samples",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Stream,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                0,
                0,
                QVariant(),
                {},
                true,
                6
            },
            250,
            true
        },
        {
            {
                "samplingPeriod",
                "Sampling Period",
                "ADC sampling period configured on the device.",
                "us",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::ADC,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                1.0,
                0.0,
                QVariant(),
                {},
                true,
                7
            },
            1.0,
            false
        },
        {
            {
                "epEnabled",
                "EP Enabled",
                "Enables or disables energy point processing.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::EnergyPoint,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Device,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                8
            },
            false,
            false
        },
        {
            {
                "adcResolution",
                "ADC Resolution",
                "Detected ADC resolution. This value is read-only.",
                "bit",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::ADC,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                0,
                QVariant(),
                QVariant(),
                {"10", "12", "14", "16"},
                true,
                9
            },
            0,
            false
        },
        {
            {
                "adcType",
                "ADC Type",
                "Detected ADC type used by the device.",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::ADC,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                "Unknown",
                QVariant(),
                QVariant(),
                {"Unknown", "Internal", "External"},
                true,
                10
            },
            "Unknown",
            false
        },
        {
            {
                "overVoltageValue",
                "Over Voltage Protection",
                "Over Voltage Protection Threshold Value",
                "V",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Protection,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                4.2,
                1.0,
                5.0,
                {},
                true,
                11
            },
            0,
            false
        },
        {
            {
                "underVoltageValue",
                "Under Voltage Protection",
                "Under Voltage Protection Threshold Value",
                "V",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Protection,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                3.2,
                1.0,
                5.0,
                {},
                true,
                12
            },
            0,
            false
        },
        {
            {
                "bdSize",
                "EEPROM Memory size",
                "Non Volatile memory size",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::FileStorage,
                Params::Access::ReadOnly,
                Params::Storage::None,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                13
            },
            0,
            false
        },
        {
            {
                "overCurrentValue",
                "Over Current Protection",
                "Over Current Protection Threshold Value",
                "mA",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Protection,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                1000,
                5000,
                0,
                {},
                true,
                14
            },
            0,
            false
        },
        {
            {
                "adcVRef",
                "ADC Voltage Ref",
                "ADC Voltage Reference voltage",
                "V",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                15
            },
            0,
            false
        },
        {
            {
                "adcVOff",
                "ADC Voltage Offset",
                "ADC Voltage Offset used during processing on host side",
                "V",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                16
            },
            0,
            false
        },
        {
            {
                "adcVCor",
                "ADC Voltage Correction",
                "ADC Voltage Correction used during processing on host side",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                17
            },
            0,
            false
        },
        {
            {
                "adcVCOffset",
                "ADC Voltage Current offset",
                "Voltage offset for bidirectional measurement",
                "V",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                18
            },
            0,
            false
        },
        {
            {
                "adcCCor",
                "Current Correction",
                "Current correction",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                19
            },
            0,
            false
        },
        {
            {
                "shuntValue",
                "Current Sensing Shunt",
                "Current Sensing Shunt",
                "Ohm",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                20
            },
            0,
            false
        },
        {
            {
                "gainValue",
                "OP Amplifier Gain",
                "Current Operational Amplifier Gain",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::Calibration,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Device,
                {},
                {},
                {},
                {},
                true,
                21
            },
            0,
            false
        },
        {
            {
                "fsPresent",
                "File System Mounted",
                "File System Mounted",
                "",
                DeviceParamDefs::DeviceConfig,
                DeviceParamDefs::FileStorage,
                Params::Access::ReadOnly,
                Params::Storage::None,
                Params::Target::Device,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                22
            },
            0,
            false
        },
        {
            {
                "maxNumberOfBuffers",
                "Max Number of Buffers",
                "Maximum number of sample buffers collected by the application.",
                "",
                Group::ApplicationConfig,
                DeviceParamDefs::Processing,
                Params::Access::ReadWrite,
                Params::Storage::LoadSave,
                Params::Target::Application,
                0,
                0,
                QVariant(),
                {},
                true,
                23
            },
            5,
            true
        },
        {
            {
                "consumptionType",
                "Consumption Type",
                "Application-side consumption calculation mode.",
                "",
                Group::ApplicationConfig,
                DeviceParamDefs::Processing,
                Params::Access::ReadOnly,
                Params::Storage::LoadSave,
                Params::Target::Application,
                "Combined",
                QVariant(),
                QVariant(),
                {"Combined"},
                true,
                24
            },
            "Combined",
            false
        },
        {
            {
                "streamId",
                "Stream ID",
                "Runtime stream identifier created by the device.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Stream,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                -1,
                QVariant(),
                QVariant(),
                {},
                true,
                25
            },
            -1,
            false
        },
        {
            {
                "powerPathState",
                "Power Path State",
                "Current power path state.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Battery,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                26
            },
            false,
            false
        },
        {
            {
                "loadState",
                "Load State",
                "Current load enable state.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Load,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                27
            },
            false,
            false
        },
        {
            {
                "loadCurrent",
                "Load Current",
                "Current load discharge current.",
                "mA",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Load,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                0,
                0,
                QVariant(),
                {},
                true,
                28
            },
            0,
            false
        },
        {
            {
                "dacState",
                "DAC State",
                "Current DAC enable state.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Load,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                29
            },
            false,
            false
        },
        {
            {
                "batteryState",
                "Battery State",
                "Current battery path state.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Battery,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                30
            },
            false,
            false
        },
        {
            {
                "underVoltageStatus",
                "Under Voltage Status",
                "Under-voltage protection indication.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Protection,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                31
            },
            false,
            false
        },
        {
            {
                "overVoltageStatus",
                "Over Voltage Status",
                "Over-voltage protection indication.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Protection,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                32
            },
            false,
            false
        },
        {
            {
                "overCurrentStatus",
                "Over Current Status",
                "Over-current protection indication.",
                "",
                DeviceParamDefs::RuntimeState,
                DeviceParamDefs::Protection,
                Params::Access::RuntimeOnly,
                Params::Storage::None,
                Params::Target::Runtime,
                false,
                QVariant(),
                QVariant(),
                {"false", "true"},
                true,
                33
            },
            false,
            false
        },
        {
            {
                "calculatedAdcSamplingTime",
                "Calculated ADC Sampling Time",
                "ADC sampling time calculated by the application.",
                "ms",
                DeviceParamDefs::Calculated,
                DeviceParamDefs::ADC,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::Calculated,
                0.0,
                0.0,
                QVariant(),
                {},
                true,
                34
            },
            0.0,
            false
        },
        {
            {
                "chargerSerial",
                "Serial Number",
                "Charger Serial Number",
                "",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerGeneral,
                Params::Access::ReadOnly,
                Params::Storage::SaveOnly,
                Params::Target::ChargerConfig,
                {},
                {},
                {},
                {},
                true,
                35
            },
            "",
            false
        },
        {
            {
                "chargerFwVersion",
                "Firmware Version",
                "Charger Firmware Version",
                "",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerGeneral,
                Params::Access::ReadOnly,
                Params::Storage::LoadSave,
                Params::Target::ChargerConfig,
                {},
                {},
                {},
                {},
                true,
                36
            },
            "",
            false
        },
        {
            {
                "chargerTermVoltage",
                "Termination Voltage",
                "Charger Termination Voltage that start CV charging phase",
                "V",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerConf,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::ChargerConfig,
                4.2,
                1.0,
                5.0,
                {},
                true,
                37
            },
            0,
            false
        },
        {
            {
                "chargerTermCurrent",
                "Termination Current",
                "Charger Termination Current in percentage of Charge current",
                "%",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerConf,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::ChargerConfig,
                20,
                10,
                80,
                {"0","5", "10", "20"},
                true,
                38,
                Params::Editor::ComboBox
            },
            0,
            false
        },
        {
            {
                "chargerChargeCurrent",
                "Charging Current",
                "Charging Current",
                "mA",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerConf,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::ChargerConfig,
                100,
                10,
                1000,
                {},
                true,
                39
            },
            0,
            false
        },
        {
            {
                "chargerMaxChargeCurrent",
                "Max Charging Current",
                "Max Charging Current",
                "mA",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerConf,
                Params::Access::ReadWrite,
                Params::Storage::SaveOnly,
                Params::Target::ChargerConfig,
                500,
                50,
                1100,
                {"50", "100", "200", "300", "400", "500", "700", "1100"},
                true,
                40,
                Params::Editor::ComboBox
            },
            0,
            false
        },
        {
            {
                "chargerBdSize",
                "EEPROM Memory size",
                "Non Volatile memory size",
                "",
                DeviceParamDefs::ChargerConfig,
                DeviceParamDefs::ChargerBD,
                Params::Access::ReadOnly,
                Params::Storage::None,
                Params::Target::ChargerConfig,
                {},
                {},
                {},
                {},
                true,
                41
            },
            0,
            false
        }
    };
}

}
