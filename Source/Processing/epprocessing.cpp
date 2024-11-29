#include "epprocessing.h"

EPProcessing::EPProcessing(QObject *parent)
    : QObject{parent}
{

    epProcessingThread = new QThread(this);
    this->moveToThread(epProcessingThread);
    epProcessingThread->setObjectName("Data processing thread");
    epProcessingThread->start();
}

void EPProcessing::onNewEPValueReceived(unsigned int PacketID, double value, double key)
{
    for(int i = 0; i < epList.size(); i++)
    {
        if(*epList[i] == PacketID)
        {
            epList[i]->assignValue(value, key);
            emit sigEPProcessed(epList[i]->getValue(), epList[i]->getKey(), epList[i]->getName());
            return;
        }
    }
    epList.append(new EPInfo(PacketID, value, key));
}

void EPProcessing::onNewEPNameReceived(unsigned int PacketID, QString name)
{
    for(int i = 0; i < epList.size(); i++)
    {
        if(*epList[i] == PacketID)
        {
            epList[i]->assignName(name);
            emit sigEPProcessed(epList[i]->getValue(), epList[i]->getKey(), epList[i]->getName());
            return;
        }
    }
    epList.append(new EPInfo(PacketID, name));
}

EPInfo::EPInfo(unsigned int aPacketID, double aValue, double aKey)
{
    packetID = aPacketID;
    value = aValue;
    key = aKey;
    nameAssigned = false;
    valueAssigned = true;
}

EPInfo::EPInfo(unsigned int aPacketID, QString aName)
{

    packetID = aPacketID;
    name = aName;
    nameAssigned = true;
    valueAssigned = false;
}

bool EPInfo::assignName(QString aName)
{
    if(nameAssigned == true) return false;
    name = aName;
    nameAssigned = true;
    return true;
}

bool EPInfo::assignValue(double aValue, double aKey)
{
    value = aValue;
    key = aKey;
    valueAssigned = true;
    return true;
}

double EPInfo::getValue()
{
    return value;
}

double EPInfo::getKey()
{
    return key;
}

QString EPInfo::getName()
{
    return name;
}

bool EPInfo::operator ==(const unsigned int &aPacketID)
{
    if(aPacketID == packetID) return true;
    return false;
}
