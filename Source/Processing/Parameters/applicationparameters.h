#ifndef APPLICATIONPARAMETERS_H
#define APPLICATIONPARAMETERS_H

#include "parameterstore.h"
#include "applicationparamdefs.h"

class ApplicationParameters : public ParameterStore
{
    Q_OBJECT

public:
    explicit ApplicationParameters(QObject *parent = nullptr);
};

#endif // APPLICATIONPARAMETERS_H
