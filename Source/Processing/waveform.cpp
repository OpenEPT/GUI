#include "waveform.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QtMath>

Waveform::Waveform()
{
    type = WAVEFORM_TYPE_CUSTOM;
    origin = WAVEFORM_ORIGIN_USER;
    repetitionCounter = WAVEFORM_COUNTER_INFINITE;
    amplitude = 0;
    period = 0;
    points = 0;
}

bool Waveform::isValid()
{
    return !chunks.isEmpty();
}

unsigned int Waveform::getTotalDuration()
{
    unsigned int total = 0;
    for(int i = 0; i < chunks.size(); i++)
    {
        int repetitions = chunks[i].repetitions > 0 ? chunks[i].repetitions : 1;
        total += chunks[i].duration * repetitions;
    }
    return total;
}

waveform_chunk_t Waveform::chunkDefault()
{
    waveform_chunk_t chunk;
    chunk.value = 0;
    chunk.valueDev = 0;
    chunk.duration = 1;
    chunk.durationDev = 0;
    chunk.repetitions = 1;
    chunk.lastInGroup = false;
    chunk.marker = "";
    chunk.markerPos = "";
    return chunk;
}

bool Waveform::chunkHasMarker(waveform_chunk_t chunk)
{
    return !chunk.marker.trimmed().isEmpty();
}

QString Waveform::chunkNormalizeMarkerPos(QString pos)
{
    QString p = pos.trimmed().toLower().remove(' ');
    if(p == "e") return "e";
    if(p == "s,e" || p == "e,s" || p == "se" || p == "es") return "s,e";
    return "s";
}

bool Waveform::chunkMarkerValid(waveform_chunk_t chunk, QString* error)
{
    if(!chunkHasMarker(chunk)) return true;
    if(chunkNormalizeMarkerPos(chunk.markerPos) != "s,e") return true;
    QStringList parts = chunk.marker.split(',');
    if(parts.size() != 2 || parts[0].trimmed().isEmpty() || parts[1].trimmed().isEmpty())
    {
        if(error != NULL) *error = "Marker \"" + chunk.marker + "\" with position s,e must be in format \"Start name, End name\"";
        return false;
    }
    return true;
}

QString Waveform::chunkToCommand(waveform_chunk_t chunk)
{
    QString command = "device wave chunk add -value=" +
            QString::number(chunk.value) + "," +
            QString::number(chunk.valueDev) + "," +
            QString::number(chunk.duration) + "," +
            QString::number(chunk.durationDev) + "," +
            QString::number(chunk.repetitions) + "," +
            QString::number(chunk.lastInGroup ? 1 : 0) + ";";
    if(chunkHasMarker(chunk))
    {
        command += " -marker=\"" + chunk.marker.trimmed() + "\" -pos=" + chunkNormalizeMarkerPos(chunk.markerPos);
    }
    return command;
}

bool Waveform::chunkFromCommand(QString line, waveform_chunk_t* chunk)
{
    QRegularExpression re("device\\s+wave\\s+chunk\\s+add\\s+-value\\s*=\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(\\d+)");
    QRegularExpression markerRe("-marker\\s*=\\s*(?:\"([^\"]*)\"|(\\S+))");
    QRegularExpression posRe("-pos\\s*=\\s*([se](?:,[se])?)");
    QRegularExpressionMatch match = re.match(line);
    if(!match.hasMatch()) return false;
    if(chunk == NULL) return false;
    *chunk = chunkDefault();
    chunk->value = match.captured(1).toUInt();
    chunk->valueDev = match.captured(2).toUInt();
    chunk->duration = match.captured(3).toUInt();
    chunk->durationDev = match.captured(4).toUInt();
    chunk->repetitions = match.captured(5).toInt();
    chunk->lastInGroup = match.captured(6).toUInt() != 0;
    QRegularExpressionMatch markerMatch = markerRe.match(line);
    if(markerMatch.hasMatch())
    {
        chunk->marker = markerMatch.captured(1).isEmpty() ? markerMatch.captured(2) : markerMatch.captured(1);
        chunk->markerPos = "s";
        QRegularExpressionMatch posMatch = posRe.match(line);
        if(posMatch.hasMatch())
        {
            chunk->markerPos = chunkNormalizeMarkerPos(posMatch.captured(1));
        }
    }
    return true;
}

QString Waveform::typeToString(waveform_type_t aType)
{
    switch(aType)
    {
    case WAVEFORM_TYPE_RAMP:
        return "Ramp";
    case WAVEFORM_TYPE_SAWTOOTH:
        return "Sawtooth";
    case WAVEFORM_TYPE_TRIANGLE:
        return "Triangle";
    case WAVEFORM_TYPE_SQUARE:
        return "Square";
    case WAVEFORM_TYPE_SINE:
        return "Sine";
    case WAVEFORM_TYPE_CUSTOM:
    default:
        return "Custom";
    }
}

waveform_type_t Waveform::typeFromString(QString aType)
{
    if(aType == "Ramp") return WAVEFORM_TYPE_RAMP;
    if(aType == "Sawtooth") return WAVEFORM_TYPE_SAWTOOTH;
    if(aType == "Triangle") return WAVEFORM_TYPE_TRIANGLE;
    if(aType == "Square") return WAVEFORM_TYPE_SQUARE;
    if(aType == "Sine") return WAVEFORM_TYPE_SINE;
    return WAVEFORM_TYPE_CUSTOM;
}

QStringList Waveform::standardTypeNames()
{
    QStringList names;
    names << "Ramp" << "Sawtooth" << "Triangle" << "Square" << "Sine";
    return names;
}

bool Waveform::generateStandard(waveform_type_t aType, unsigned int aAmplitude, unsigned int aPeriod, unsigned int aPoints, int aRepetitionCounter)
{
    unsigned int stepDuration;
    double phase;
    double value;

    chunks.clear();
    type = aType;
    origin = WAVEFORM_ORIGIN_STANDARD;
    amplitude = aAmplitude;
    period = aPeriod;
    points = aPoints;
    repetitionCounter = aRepetitionCounter;
    name = typeToString(aType) + " " + QString::number(aAmplitude) + "mA/" + QString::number(aPeriod) + "ms/" + QString::number(aPoints) + "pts";

    if(points < 2 || period == 0) return false;

    stepDuration = period / points;
    if(stepDuration == 0) stepDuration = 1;

    for(unsigned int i = 0; i < points; i++)
    {
        phase = (double)i / (double)points;
        value = 0.0;
        switch(type)
        {
        case WAVEFORM_TYPE_RAMP:
            value = amplitude * (double)i / (double)(points - 1);
            break;
        case WAVEFORM_TYPE_SAWTOOTH:
            value = amplitude * (1.0 - (double)i / (double)(points - 1));
            break;
        case WAVEFORM_TYPE_TRIANGLE:
            if(phase < 0.5)
            {
                value = amplitude * (phase * 2.0);
            }
            else
            {
                value = amplitude * (2.0 - phase * 2.0);
            }
            break;
        case WAVEFORM_TYPE_SQUARE:
            if(phase < 0.5)
            {
                value = amplitude;
            }
            else
            {
                value = 0.0;
            }
            break;
        case WAVEFORM_TYPE_SINE:
            value = (amplitude / 2.0) * (1.0 + qSin(2.0 * M_PI * phase));
            break;
        default:
            break;
        }
        waveform_chunk_t chunk = chunkDefault();
        chunk.value = (unsigned int)qRound(value);
        chunk.duration = stepDuration;
        chunk.lastInGroup = (i == points - 1);
        chunks.append(chunk);
    }
    return true;
}

QStringList Waveform::getCommands()
{
    QStringList commands;
    commands << "device wave clear";
    for(int i = 0; i < chunks.size(); i++)
    {
        commands << chunkToCommand(chunks[i]);
    }
    commands << "device wave counter set -value=" + QString::number(repetitionCounter);
    return commands;
}

QString Waveform::getFileContent()
{
    QString content;
    content += "# name: " + name + "\n";
    content += "# type: " + typeToString(type) + "\n";
    if(origin == WAVEFORM_ORIGIN_STANDARD)
    {
        content += "# amplitude: " + QString::number(amplitude) + " mA, period: " + QString::number(period) + " ms, points: " + QString::number(points) + "\n";
    }
    content += "\n";
    for(int i = 0; i < chunks.size(); i++)
    {
        content += chunkToCommand(chunks[i]) + "\n";
    }
    content += "device wave counter set -value=" + QString::number(repetitionCounter) + ";\n";
    return content;
}

bool Waveform::parseContent(QString content, QString* error)
{
    QRegularExpression counterRe("device\\s+wave\\s+counter\\s+set\\s+-value\\s*=\\s*(-?\\d+)");
    QRegularExpression nameRe("^#\\s*name\\s*:\\s*(.+)$");
    QRegularExpression typeRe("^#\\s*type\\s*:\\s*(\\w+)");
    QStringList lines = content.split('\n');
    waveform_chunk_t chunk;

    chunks.clear();
    for(int i = 0; i < lines.size(); i++)
    {
        QString line = lines[i].trimmed();
        if(line.isEmpty()) continue;

        if(line.startsWith('#'))
        {
            QRegularExpressionMatch nameMatch = nameRe.match(line);
            if(nameMatch.hasMatch())
            {
                name = nameMatch.captured(1).trimmed();
            }
            QRegularExpressionMatch typeMatch = typeRe.match(line);
            if(typeMatch.hasMatch())
            {
                type = typeFromString(typeMatch.captured(1));
            }
            continue;
        }

        if(chunkFromCommand(line, &chunk))
        {
            chunks.append(chunk);
            continue;
        }

        QRegularExpressionMatch counterMatch = counterRe.match(line);
        if(counterMatch.hasMatch())
        {
            repetitionCounter = counterMatch.captured(1).toInt();
            continue;
        }

        if(line.startsWith("device wave clear"))
        {
            chunks.clear();
            continue;
        }

        if(error != NULL) *error = "Unrecognized line " + QString::number(i + 1) + ": " + line;
        return false;
    }

    if(chunks.isEmpty())
    {
        if(error != NULL) *error = "File does not contain any wave chunk";
        return false;
    }

    chunks.last().lastInGroup = true;
    return true;
}

bool Waveform::loadFromFile(QString path, QString* error)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if(error != NULL) *error = "Unable to open file: " + path;
        return false;
    }
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    origin = WAVEFORM_ORIGIN_USER;
    type = WAVEFORM_TYPE_CUSTOM;
    if(name.isEmpty())
    {
        name = QFileInfo(path).completeBaseName();
    }
    return parseContent(content, error);
}

bool Waveform::saveToFile(QString path, QString* error)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        if(error != NULL) *error = "Unable to write file: " + path;
        return false;
    }
    file.write(getFileContent().toUtf8());
    file.close();
    return true;
}

QJsonObject Waveform::toJson()
{
    QJsonObject obj;
    QJsonArray chunksArray;

    obj["name"] = name;
    obj["type"] = typeToString(type);
    obj["origin"] = (origin == WAVEFORM_ORIGIN_STANDARD) ? "standard" : "user";
    obj["repetitionCounter"] = repetitionCounter;
    obj["amplitude"] = (int)amplitude;
    obj["period"] = (int)period;
    obj["points"] = (int)points;
    for(int i = 0; i < chunks.size(); i++)
    {
        QJsonObject chunkObj;
        chunkObj["value"] = (int)chunks[i].value;
        chunkObj["valueDev"] = (int)chunks[i].valueDev;
        chunkObj["duration"] = (int)chunks[i].duration;
        chunkObj["durationDev"] = (int)chunks[i].durationDev;
        chunkObj["repetitions"] = chunks[i].repetitions;
        chunkObj["lastInGroup"] = chunks[i].lastInGroup;
        chunkObj["marker"] = chunks[i].marker;
        chunkObj["markerPos"] = chunks[i].markerPos;
        chunksArray.append(chunkObj);
    }
    obj["chunks"] = chunksArray;
    return obj;
}

bool Waveform::fromJson(QJsonObject obj)
{
    QJsonArray chunksArray = obj["chunks"].toArray();

    name = obj["name"].toString();
    type = typeFromString(obj["type"].toString());
    origin = (obj["origin"].toString() == "standard") ? WAVEFORM_ORIGIN_STANDARD : WAVEFORM_ORIGIN_USER;
    repetitionCounter = obj["repetitionCounter"].toInt(WAVEFORM_COUNTER_INFINITE);
    amplitude = obj["amplitude"].toInt();
    period = obj["period"].toInt();
    points = obj["points"].toInt();
    chunks.clear();
    for(int i = 0; i < chunksArray.size(); i++)
    {
        QJsonObject chunkObj = chunksArray[i].toObject();
        waveform_chunk_t chunk = chunkDefault();
        chunk.value = chunkObj["value"].toInt();
        chunk.valueDev = chunkObj["valueDev"].toInt();
        chunk.duration = chunkObj["duration"].toInt(1);
        chunk.durationDev = chunkObj["durationDev"].toInt();
        chunk.repetitions = chunkObj["repetitions"].toInt(1);
        chunk.lastInGroup = chunkObj["lastInGroup"].toBool();
        chunk.marker = chunkObj["marker"].toString();
        chunk.markerPos = chunkObj["markerPos"].toString();
        chunks.append(chunk);
    }
    return !chunks.isEmpty();
}

WaveformLibrary& WaveformLibrary::instance()
{
    static WaveformLibrary library;
    return library;
}

WaveformLibrary::WaveformLibrary(QObject *parent) : QObject(parent)
{
    load();
    ensureDefaults();
}

void WaveformLibrary::ensureDefaults()
{
    QStringList typeNames = Waveform::standardTypeNames();

    if(!getNames(WAVEFORM_ORIGIN_STANDARD).isEmpty()) return;
    for(int i = 0; i < typeNames.size(); i++)
    {
        Waveform wave;
        wave.generateStandard(Waveform::typeFromString(typeNames[i]), 1000, 20, 20, WAVEFORM_COUNTER_INFINITE);
        waves.append(wave);
    }
}

QList<Waveform> WaveformLibrary::getWaves()
{
    return waves;
}

QStringList WaveformLibrary::getNames()
{
    QStringList names;
    for(int i = 0; i < waves.size(); i++)
    {
        names << waves[i].name;
    }
    return names;
}

QStringList WaveformLibrary::getNames(waveform_origin_t aOrigin)
{
    QStringList names;
    for(int i = 0; i < waves.size(); i++)
    {
        if(waves[i].origin == aOrigin)
        {
            names << waves[i].name;
        }
    }
    return names;
}

bool WaveformLibrary::contains(QString aName)
{
    for(int i = 0; i < waves.size(); i++)
    {
        if(waves[i].name == aName) return true;
    }
    return false;
}

bool WaveformLibrary::get(QString aName, Waveform* wave)
{
    if(wave == NULL) return false;
    for(int i = 0; i < waves.size(); i++)
    {
        if(waves[i].name == aName)
        {
            *wave = waves[i];
            return true;
        }
    }
    return false;
}

bool WaveformLibrary::addOrReplace(Waveform wave)
{
    bool replaced = false;
    for(int i = 0; i < waves.size(); i++)
    {
        if(waves[i].name == wave.name)
        {
            waves[i] = wave;
            replaced = true;
            break;
        }
    }
    if(!replaced)
    {
        waves.append(wave);
    }
    save();
    emit sigLibraryChanged();
    return true;
}

bool WaveformLibrary::remove(QString aName)
{
    for(int i = 0; i < waves.size(); i++)
    {
        if(waves[i].name == aName)
        {
            waves.removeAt(i);
            save();
            emit sigLibraryChanged();
            return true;
        }
    }
    return false;
}

QString WaveformLibrary::getStoragePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/waveforms.json";
}

bool WaveformLibrary::load(QString* error)
{
    QFile file(getStoragePath());
    if(!file.exists()) return true;
    if(!file.open(QIODevice::ReadOnly))
    {
        if(error != NULL) *error = "Unable to open " + getStoragePath();
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if(!doc.isArray())
    {
        if(error != NULL) *error = "Invalid waveform library format";
        return false;
    }
    QJsonArray array = doc.array();
    waves.clear();
    for(int i = 0; i < array.size(); i++)
    {
        Waveform wave;
        if(wave.fromJson(array[i].toObject()))
        {
            waves.append(wave);
        }
    }
    return true;
}

bool WaveformLibrary::save(QString* error)
{
    QJsonArray array;
    for(int i = 0; i < waves.size(); i++)
    {
        array.append(waves[i].toJson());
    }
    QFile file(getStoragePath());
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if(error != NULL) *error = "Unable to write " + getStoragePath();
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
