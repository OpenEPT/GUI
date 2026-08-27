#include "parameterstore.h"

#include <algorithm>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

ParameterStore::ParameterStore(QObject *parent)
    : QObject(parent)
{
}

ParameterStore::ParameterStore(const QList<Params::GroupMeta> &groups,
                               const QList<Params::SubGroupMeta> &subGroups,
                               const QList<Params::Param> &params,
                               QObject *parent)
    : QObject(parent)
{
    setDefinition(groups, subGroups, params);
}

void ParameterStore::setDefinition(const QList<Params::GroupMeta> &groups,
                                   const QList<Params::SubGroupMeta> &subGroups,
                                   const QList<Params::Param> &params)
{
    m_groupMeta.clear();
    m_subGroupMeta.clear();
    m_params.clear();

    for(const auto &group : groups)
    {
        m_groupMeta.insert(group.id, group);
    }

    for(const auto &subGroup : subGroups)
    {
        m_subGroupMeta.insert(subGroup.id, subGroup);
    }

    for(const auto &param : params)
    {
        m_params.insert(param.meta.key, param);
    }
}

bool ParameterStore::hasParam(const QString &key) const
{
    return m_params.contains(key);
}

QString ParameterStore::getParamValue(const QString &key) const
{
    if(!m_params.contains(key))
        return QString();

    const QVariant value = m_params.value(key).value;

    if(value.type() == QVariant::Bool)
        return value.toBool() ? "true" : "false";

    return value.toString();
}

QVariant ParameterStore::getParamVariant(const QString &key) const
{
    if(!m_params.contains(key))
        return QVariant();

    return m_params.value(key).value;
}

bool ParameterStore::setParamValue(const QString &key, const QString &value)
{
    return setParamVariant(key, value);
}

bool ParameterStore::isInitialized(const QString &key) const
{
    if(!m_params.contains(key))
        return false;

    return m_params.value(key).initialized;
}

void ParameterStore::setParamInitialized(const QString &key, bool initialized)
{
    if(!m_params.contains(key))
        return;

    m_params[key].initialized = initialized;

    emit paramChanged(key, getParamValue(key));
    emit paramVariantChanged(key, m_params[key].value);
}

bool ParameterStore::setParamVariant(const QString &key, const QVariant &value)
{
    if(!m_params.contains(key))
        return false;

    Params::Param &param = m_params[key];

    bool normalizeOk = false;
    QVariant normalizedValue = normalizeValue(param.meta, value, &normalizeOk);

    if(!normalizeOk)
        return false;

    if(!validateValue(param.meta, normalizedValue))
        return false;

    if((param.value == normalizedValue) && (param.initialized != false))
        return true;

    param.value = normalizedValue;
    param.initialized = true;

    emit paramChanged(key, getParamValue(key));
    emit paramVariantChanged(key, param.value);

    return true;
}

Params::ParamMeta ParameterStore::getParamMeta(const QString &key) const
{
    if(!m_params.contains(key))
        return Params::ParamMeta();

    return m_params.value(key).meta;
}

Params::Param ParameterStore::getParam(const QString &key) const
{
    if(!m_params.contains(key))
        return Params::Param();

    return m_params.value(key);
}

Params::Param& ParameterStore::getParamRef(const QString &key)
{
    return m_params[key];
}

QList<Params::Param> ParameterStore::getAllParams() const
{
    QList<Params::Param> result = m_params.values();

    std::sort(result.begin(), result.end(),
              [](const Params::Param &a, const Params::Param &b)
              {
                  if(a.meta.group != b.meta.group)
                      return a.meta.group < b.meta.group;

                  if(a.meta.subGroup != b.meta.subGroup)
                      return a.meta.subGroup < b.meta.subGroup;

                  return a.meta.order < b.meta.order;
              });

    return result;
}

QList<Params::Param> ParameterStore::getParamsByGroup(Params::GroupId group) const
{
    QList<Params::Param> result;

    for(const auto &param : m_params)
    {
        if(param.meta.group == group)
            result.append(param);
    }

    std::sort(result.begin(), result.end(),
              [](const Params::Param &a, const Params::Param &b)
              {
                  if(a.meta.subGroup != b.meta.subGroup)
                      return a.meta.subGroup < b.meta.subGroup;

                  return a.meta.order < b.meta.order;
              });

    return result;
}

QList<Params::Param> ParameterStore::getParamsBySubGroup(Params::GroupId group,
                                                         Params::SubGroupId subGroup) const
{
    QList<Params::Param> result;

    for(const auto &param : m_params)
    {
        if((param.meta.group == group) && (param.meta.subGroup == subGroup))
            result.append(param);
    }

    std::sort(result.begin(), result.end(),
              [](const Params::Param &a, const Params::Param &b)
              {
                  return a.meta.order < b.meta.order;
              });

    return result;
}

Params::GroupMeta ParameterStore::getGroupMeta(Params::GroupId group) const
{
    return m_groupMeta.value(group);
}

Params::SubGroupMeta ParameterStore::getSubGroupMeta(Params::SubGroupId subGroup) const
{
    return m_subGroupMeta.value(subGroup);
}

QList<Params::GroupMeta> ParameterStore::getAllGroupMeta() const
{
    QList<Params::GroupMeta> result = m_groupMeta.values();

    std::sort(result.begin(), result.end(),
              [](const Params::GroupMeta &a, const Params::GroupMeta &b)
              {
                  return a.order < b.order;
              });

    return result;
}

QList<Params::SubGroupMeta> ParameterStore::getAllSubGroupMeta() const
{
    QList<Params::SubGroupMeta> result = m_subGroupMeta.values();

    std::sort(result.begin(), result.end(),
              [](const Params::SubGroupMeta &a, const Params::SubGroupMeta &b)
              {
                  return a.order < b.order;
              });

    return result;
}

bool ParameterStore::isEditable(const QString &key) const
{
    if(!m_params.contains(key))
        return false;

    return m_params.value(key).meta.access == Params::Access::ReadWrite;
}

bool ParameterStore::isVisible(const QString &key) const
{
    if(!m_params.contains(key))
        return false;

    return m_params.value(key).meta.visible;
}

bool ParameterStore::isSavable(const QString &key) const
{
    if(!m_params.contains(key))
        return false;

    const auto storage = m_params.value(key).meta.storage;

    return storage == Params::Storage::SaveOnly ||
           storage == Params::Storage::LoadSave;
}

bool ParameterStore::isLoadable(const QString &key) const
{
    if(!m_params.contains(key))
        return false;

    return m_params.value(key).meta.storage == Params::Storage::LoadSave;
}

bool ParameterStore::isRuntimeOnly(const QString &key) const
{
    if(!m_params.contains(key))
        return false;

    return m_params.value(key).meta.access == Params::Access::RuntimeOnly;
}

void ParameterStore::resetDefaults(bool markInitialized)
{
    for(auto it = m_params.begin(); it != m_params.end(); ++it)
    {
        it.value().value = it.value().meta.defaultValue;
        it.value().initialized = markInitialized;

        emit paramChanged(it.key(), getParamValue(it.key()));
        emit paramVariantChanged(it.key(), it.value().value);
    }
}

QJsonObject ParameterStore::toJson(bool includeSaveOnly) const
{
    QJsonObject object;

    for(auto it = m_params.constBegin(); it != m_params.constEnd(); ++it)
    {
        const auto &param = it.value();

        if(param.meta.storage == Params::Storage::None)
            continue;

        if(!includeSaveOnly && param.meta.storage == Params::Storage::SaveOnly)
            continue;

        object.insert(param.meta.key, QJsonValue::fromVariant(param.value));
    }

    return object;
}

bool ParameterStore::fromJson(const QJsonObject &object)
{
    bool allOk = true;

    for(auto it = object.constBegin(); it != object.constEnd(); ++it)
    {
        const QString key = it.key();

        if(!m_params.contains(key))
        {
            allOk = false;
            continue;
        }

        if(!isLoadable(key))
            continue;

        if(!setParamVariant(key, it.value().toVariant()))
            allOk = false;
    }

    return allOk;
}

bool ParameterStore::loadValuesFromFile(const QString &filePath)
{
    QFile file(filePath);

    if(!file.exists())
        return false;

    if(!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if(parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;

    return fromJson(document.object());
}

bool ParameterStore::saveValuesToFile(const QString &filePath, bool includeSaveOnly) const
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    const QJsonDocument document(toJson(includeSaveOnly));
    file.write(document.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

bool ParameterStore::updateDatabaseFromJsonFile(const QString &filePath)
{
    QFile file(filePath);

    if(!file.exists())
        return false;

    if(!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if(parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;

    return updateDatabaseFromJson(document.object());
}

bool ParameterStore::updateDatabaseFromJson(const QJsonObject &object)
{
    if(!object.contains("parameters") || !object.value("parameters").isArray())
        return false;

    bool allOk = true;
    const QJsonArray paramsArray = object.value("parameters").toArray();

    for(const QJsonValue &value : paramsArray)
    {
        if(!value.isObject())
        {
            allOk = false;
            continue;
        }

        const QJsonObject paramObject = value.toObject();
        const QString key = paramObject.value("key").toString();

        if(key.isEmpty() || !m_params.contains(key))
        {
            allOk = false;
            continue;
        }

        if(!updateParamMetaFromJson(m_params[key], paramObject))
            allOk = false;
    }

    return allOk;
}

bool ParameterStore::updateParamMetaFromJson(Params::Param &param,
                                             const QJsonObject &object)
{
    Params::ParamMeta meta = param.meta;

    if(object.contains("displayName"))
        meta.displayName = object.value("displayName").toString(meta.displayName);

    if(object.contains("description"))
        meta.description = object.value("description").toString(meta.description);

    if(object.contains("unit"))
        meta.unit = object.value("unit").toString(meta.unit);

    if(object.contains("defaultValue"))
        meta.defaultValue = object.value("defaultValue").toVariant();

    if(object.contains("minValue"))
        meta.minValue = object.value("minValue").toVariant();

    if(object.contains("maxValue"))
        meta.maxValue = object.value("maxValue").toVariant();

    if(object.contains("visible"))
        meta.visible = object.value("visible").toBool(meta.visible);

    if(object.contains("order"))
        meta.order = object.value("order").toInt(meta.order);

    if(object.contains("allowedValues") && object.value("allowedValues").isArray())
    {
        QStringList allowedValues;
        const QJsonArray allowedArray = object.value("allowedValues").toArray();

        for(const QJsonValue &allowedValue : allowedArray)
        {
            allowedValues.append(allowedValue.toString());
        }

        meta.allowedValues = allowedValues;
    }

    if(object.contains("access"))
        meta.access = parseAccess(object.value("access").toString(), meta.access);

    if(object.contains("storage"))
        meta.storage = parseStorage(object.value("storage").toString(), meta.storage);

    if(object.contains("target"))
        meta.target = parseTarget(object.value("target").toString(), meta.target);

    param.meta = meta;

    if(object.contains("defaultValue") && !param.initialized)
        param.value = meta.defaultValue;

    return true;
}

Params::Access ParameterStore::parseAccess(const QString &value,
                                           Params::Access fallback) const
{
    const QString normalized = value.trimmed().toLower();

    if(normalized == "readwrite") return Params::Access::ReadWrite;
    if(normalized == "readonly") return Params::Access::ReadOnly;
    if(normalized == "runtimeonly") return Params::Access::RuntimeOnly;

    return fallback;
}

Params::Storage ParameterStore::parseStorage(const QString &value,
                                             Params::Storage fallback) const
{
    const QString normalized = value.trimmed().toLower();

    if(normalized == "none") return Params::Storage::None;
    if(normalized == "saveonly") return Params::Storage::SaveOnly;
    if(normalized == "loadsave") return Params::Storage::LoadSave;

    return fallback;
}

Params::Target ParameterStore::parseTarget(const QString &value,
                                           Params::Target fallback) const
{
    const QString normalized = value.trimmed().toLower();

    if(normalized == "device") return Params::Target::Device;
    if(normalized == "application") return Params::Target::Application;
    if(normalized == "runtime") return Params::Target::Runtime;
    if(normalized == "calculated") return Params::Target::Calculated;

    return fallback;
}

bool ParameterStore::validateValue(const Params::ParamMeta &meta,
                                   const QVariant &value) const
{
    if(!meta.allowedValues.isEmpty())
    {
        if(!meta.allowedValues.contains(value.toString(), Qt::CaseInsensitive))
            return false;
    }

    if(meta.minValue.isValid())
    {
        bool okValue = false;
        bool okMin = false;

        const double v = value.toDouble(&okValue);
        const double min = meta.minValue.toDouble(&okMin);

        if(okValue && okMin && v < min)
            return false;
    }

    if(meta.maxValue.isValid())
    {
        bool okValue = false;
        bool okMax = false;

        const double v = value.toDouble(&okValue);
        const double max = meta.maxValue.toDouble(&okMax);

        if(okValue && okMax && v > max)
            return false;
    }

    return true;
}

QVariant ParameterStore::normalizeValue(const Params::ParamMeta &meta,
                                        const QVariant &value,
                                        bool *ok) const
{
    if(ok)
        *ok = true;

    const QVariant defaultValue = meta.defaultValue;

    if(!defaultValue.isValid())
        return value;

    switch(defaultValue.type())
    {
    case QVariant::Bool:
    {
        if(value.type() == QVariant::Bool)
            return value.toBool();

        const QString str = value.toString().trimmed().toLower();

        if(str == "true" || str == "1" || str == "yes" || str == "enabled")
            return true;

        if(str == "false" || str == "0" || str == "no" || str == "disabled")
            return false;

        if(ok)
            *ok = false;

        return QVariant();
    }

    case QVariant::Int:
    {
        bool conversionOk = false;
        const int converted = value.toInt(&conversionOk);

        if(ok)
            *ok = conversionOk;

        return converted;
    }

    case QVariant::UInt:
    {
        bool conversionOk = false;
        const uint converted = value.toUInt(&conversionOk);

        if(ok)
            *ok = conversionOk;

        return converted;
    }

    case QVariant::LongLong:
    {
        bool conversionOk = false;
        const qlonglong converted = value.toLongLong(&conversionOk);

        if(ok)
            *ok = conversionOk;

        return converted;
    }

    case QVariant::ULongLong:
    {
        bool conversionOk = false;
        const qulonglong converted = value.toULongLong(&conversionOk);

        if(ok)
            *ok = conversionOk;

        return converted;
    }

    case QVariant::Double:
    {
        bool conversionOk = false;
        const double converted = value.toDouble(&conversionOk);

        if(ok)
            *ok = conversionOk;

        return converted;
    }

    case QVariant::String:
        return value.toString();

    default:
        return value;
    }
}
