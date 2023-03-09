#pragma once

#include <QString>
#include <QLibrary>
#include "dga_parameters.h"

typedef void (*FuncPrototype)(char *, int, int, void *, void *, void *);

class DgaLibrary
{
public:
    virtual ~DgaLibrary();

    bool load();
    QString getParameter(int par);

private:
    QLibrary *lib = nullptr;
    FuncPrototype func = nullptr;
};
