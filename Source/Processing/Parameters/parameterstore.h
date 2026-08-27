#ifndef PARAMETERSTORE_H
#define PARAMETERSTORE_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QString>
#include <QVariant>

#include "parameterdefs.h"

class ParameterStore : public QObject
{
    Q_OBJECT

public:
    explicit ParameterStore(QObject *parent = nullptr);
    explicit ParameterStore(const QList<Params::GroupMeta> &groups,
                            const QList<Params::SubGroupMeta> &subGroups,
                            const QList<Params::Param> &params,
                            QObject *parent = nullptr);

    void setDefinition(const QList<Params::GroupMeta> &groups,
                       const QList<Params::SubGroupMeta> &subGroups,
                       const QList<Params::Param> &params);

    bool hasParam(const QString &key) const;

    QString getParamValue(const QString &key) const;
    QVariant getParamVariant(const QString &key) const;

    bool setParamValue(const QString &key, const QString &value);
    bool setParamVariant(const QString &key, const QVariant &value);

    bool isInitialized(const QString &key) const;
    void setParamInitialized(const QString &key, bool initialized);

    Params::ParamMeta getParamMeta(const QString &key) const;
    Params::Param getParam(const QString &key) const;
    Params::Param& getParamRef(const QString &key);

    QList<Params::Param> getAllParams() const;
    QList<Params::Param> getParamsByGroup(Params::GroupId group) const;
    QList<Params::Param> getParamsBySubGroup(Params::GroupId group,
                                             Params::SubGroupId subGroup) const;

    Params::GroupMeta getGroupMeta(Params::GroupId group) const;
    Params::SubGroupMeta getSubGroupMeta(Params::SubGroupId subGroup) const;

    QList<Params::GroupMeta> getAllGroupMeta() const;
    QList<Params::SubGroupMeta> getAllSubGroupMeta() const;

    bool isEditable(const QString &key) const;
    bool isVisible(const QString &key) const;
    bool isSavable(const QString &key) const;
    bool isLoadable(const QString &key) const;
    bool isRuntimeOnly(const QString &key) const;

    void resetDefaults(bool markInitialized = true);

    QJsonObject toJson(bool includeSaveOnly = true) const;
    bool fromJson(const QJsonObject &object);

    bool loadValuesFromFile(const QString &filePath);
    bool saveValuesToFile(const QString &filePath, bool includeSaveOnly = true) const;

    /*
     * Optional parameter database override.
     * Expected JSON format:
     * {
     *   "parameters": [
     *     {
     *       "key": "streamServiceBasePort",
     *       "displayName": "Stream Service Base Port",
     *       "description": "...",
     *       "unit": "",
     *       "defaultValue": 11223,
     *       "minValue": 0,
     *       "maxValue": 65535,
     *       "allowedValues": [],
     *       "visible": true,
     *       "order": 0,
     *       "access": "ReadWrite",
     *       "storage": "LoadSave",
     *       "target": "Application"
     *     }
     *   ]
     * }
     */
    bool updateDatabaseFromJsonFile(const QString &filePath);
    bool updateDatabaseFromJson(const QJsonObject &object);

signals:
    void paramChanged(QString key, QString value);
    void paramVariantChanged(QString key, QVariant value);

private:
    bool validateValue(const Params::ParamMeta &meta,
                       const QVariant &value) const;

    QVariant normalizeValue(const Params::ParamMeta &meta,
                            const QVariant &value,
                            bool *ok = nullptr) const;

    bool updateParamMetaFromJson(Params::Param &param,
                                 const QJsonObject &object);

    Params::Access parseAccess(const QString &value,
                               Params::Access fallback) const;
    Params::Storage parseStorage(const QString &value,
                                 Params::Storage fallback) const;
    Params::Target parseTarget(const QString &value,
                               Params::Target fallback) const;

private:
    QMap<QString, Params::Param> m_params;
    QMap<Params::GroupId, Params::GroupMeta> m_groupMeta;
    QMap<Params::SubGroupId, Params::SubGroupMeta> m_subGroupMeta;
};

#endif // PARAMETERSTORE_H
