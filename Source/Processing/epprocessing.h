#ifndef EPPROCESSING_H
#define EPPROCESSING_H

#include <QObject>
#include <QThread>

class EPInfo
{
public:
    explicit EPInfo(unsigned int aPacketID=0, double aValue=0, double aKey=0);
    explicit EPInfo(unsigned int aPacketID, QString aName);

    bool     assignName(QString aName);
    bool     assignValue(double aValue=0, double aKey=0);

    double   getValue();
    double   getKey();
    QString  getName();


    bool operator ==(const unsigned int& aPacketID);

private:
    unsigned int packetID;
    double value;
    double key;
    QString name;
    bool nameAssigned;
    bool valueAssigned;
};

class EPProcessing : public QObject
{
    Q_OBJECT
public:
    explicit EPProcessing(QObject *parent = nullptr);

signals:

    void    sigEPProcessed(double value, double key, QString name);

public slots:
    void    onNewEPValueReceived(unsigned int PacketID, double value, double key);
    void    onNewEPNameReceived(unsigned int PacketID, QString name);

private:
    QThread                          *epProcessingThread;
    QVector<EPInfo*>                 epList;

};

#endif // EPPROCESSING_H
