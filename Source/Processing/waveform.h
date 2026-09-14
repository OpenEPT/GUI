#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonObject>

#define WAVEFORM_CHUNK_MAX_NO           200
#define WAVEFORM_COUNTER_INFINITE       -1

typedef enum
{
    WAVEFORM_TYPE_RAMP = 0,
    WAVEFORM_TYPE_SAWTOOTH,
    WAVEFORM_TYPE_TRIANGLE,
    WAVEFORM_TYPE_SQUARE,
    WAVEFORM_TYPE_SINE,
    WAVEFORM_TYPE_CUSTOM
}waveform_type_t;

typedef enum
{
    WAVEFORM_ORIGIN_STANDARD = 0,
    WAVEFORM_ORIGIN_USER
}waveform_origin_t;

typedef struct
{
    unsigned int    value;
    unsigned int    valueDev;
    unsigned int    duration;
    unsigned int    durationDev;
    int             repetitions;
    bool            lastInGroup;
}waveform_chunk_t;

class Waveform
{
public:
    explicit                Waveform();

    bool                    isValid();
    unsigned int            getTotalDuration();

    bool                    generateStandard(waveform_type_t aType, unsigned int aAmplitude, unsigned int aPeriod, unsigned int aPoints, int aRepetitionCounter);

    QStringList             getCommands();
    QString                 getFileContent();
    bool                    parseContent(QString content, QString* error = NULL);
    bool                    loadFromFile(QString path, QString* error = NULL);
    bool                    saveToFile(QString path, QString* error = NULL);

    QJsonObject             toJson();
    bool                    fromJson(QJsonObject obj);

    static QString          chunkToCommand(waveform_chunk_t chunk);
    static bool             chunkFromCommand(QString line, waveform_chunk_t* chunk);
    static waveform_chunk_t chunkDefault();

    static QString          typeToString(waveform_type_t aType);
    static waveform_type_t  typeFromString(QString aType);
    static QStringList      standardTypeNames();

public:
    QString                 name;
    waveform_type_t         type;
    waveform_origin_t       origin;
    QList<waveform_chunk_t> chunks;
    int                     repetitionCounter;

    unsigned int            amplitude;
    unsigned int            period;
    unsigned int            points;
};

class WaveformLibrary : public QObject
{
    Q_OBJECT
public:
    static WaveformLibrary& instance();

    QList<Waveform>         getWaves();
    QStringList             getNames();
    QStringList             getNames(waveform_origin_t aOrigin);
    bool                    contains(QString aName);
    bool                    get(QString aName, Waveform* wave);

    bool                    addOrReplace(Waveform wave);
    bool                    remove(QString aName);

    bool                    load(QString* error = NULL);
    bool                    save(QString* error = NULL);
    QString                 getStoragePath();

signals:
    void                    sigLibraryChanged();

private:
    explicit                WaveformLibrary(QObject *parent = nullptr);
    void                    ensureDefaults();

    QList<Waveform>         waves;
};

#endif
