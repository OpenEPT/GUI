#ifndef PARAMETERDEFS_H
#define PARAMETERDEFS_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Params
{
    using GroupId = int;
    using SubGroupId = int;

    enum class Access
    {
        ReadWrite,
        ReadOnly,
        RuntimeOnly
    };

    enum class Storage
    {
        None,
        SaveOnly,
        LoadSave
    };

    enum class Target
    {
        Device,
        Application,
        Runtime,
        Calculated,
        ChargerConfig
    };

    enum class Editor
    {
        LineEdit,
        ComboBox
    };

    struct GroupMeta
    {
        GroupId id;
        QString name;
        QString description;
        int order;
    };

    struct SubGroupMeta
    {
        SubGroupId id;
        QString name;
        QString description;
        int order;
    };

    struct ParamMeta
    {
        QString key;
        QString displayName;
        QString description;
        QString unit;

        GroupId group;
        SubGroupId subGroup;
        Access access;
        Storage storage;
        Target target;

        QVariant defaultValue;
        QVariant minValue;
        QVariant maxValue;
        QStringList allowedValues;

        bool visible;
        int order;

        Editor editor = Editor::LineEdit;
    };

    struct Param
    {
        ParamMeta meta;
        QVariant value;
        bool initialized;
        std::function<bool(const QVariant&)> setFn;
        std::function<void()> getFn;
    };
}

#endif // PARAMETERDEFS_H
