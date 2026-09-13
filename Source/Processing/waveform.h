#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonObject>

#define WAVEFORM_CHUNK_MAX_NO           200     /*Same as LOAD_WAVE_CHUNK_MAX_NO in firmware*/
#define WAVEFORM_COUNTER_INFINITE       -1

typedef enum
{
    WAVEFORM_TYPE_RAMP = 0,     /*Linear rise 0 -> A*/
    WAVEFORM_TYPE_SAWTOOTH,     /*Linear fall A -> 0*/
    WAVEFORM_TYPE_TRIANGLE,     /*Rise then fall*/
    WAVEFORM_TYPE_SQUARE,       /*A for half period, 0 for other half*/
    WAVEFORM_TYPE_SINE,         /*0..A sine*/
    WAVEFORM_TYPE_CUSTOM        /*User defined*/
}waveform_type_t;

typedef enum
{
    WAVEFORM_ORIGIN_STANDARD = 0,
    WAVEFORM_ORIGIN_USER
}waveform_origin_t;

/*
 * One wave chunk, matches firmware "device wave chunk add -value=..." format:
 * value,valueDev,duration,durationDev,repetitions,lastInGroup
 */
typedef struct
{
    unsigned int    value;          /*[mA]*/
    unsigned int    valueDev;       /*[mA]*/
    unsigned int    duration;       /*[ms]*/
    unsigned int    durationDev;    /*[ms]*/
    int             repetitions;    /*Chunk repetitions (>0)*/
    bool            lastInGroup;
}waveform_chunk_t;

class Waveform
{
public:
    explicit                Waveform();

    bool                    isValid();
    unsigned int            getTotalDuration();                     /*[ms], one pass*/

    bool                    generateStandard(waveform_type_t aType, unsigned int aAmplitude, unsigned int aPeriod, unsigned int aPoints, int aRepetitionCounter);

    QStringList             getCommands();                          /*Firmware command list (clear + chunks + counter)*/
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
    int                     repetitionCounter;                      /*-1 = infinite*/

    /*Standard wave parameters (valid when origin == WAVEFORM_ORIGIN_STANDARD)*/
    unsigned int            amplitude;                              /*[mA]*/
    unsigned int            period;                                 /*[ms]*/
    unsigned int            points;
};

/*
 * Central registry of all waves (standard + user defined). Waves stored here can
 * be reused anywhere in application (Load, Charge/Discharge process, ...).
 */
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

#endif // WAVEFORM_H
