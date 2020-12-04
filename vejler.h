#ifndef VEJLER_H
#define VEJLER_H

#include "common.h"

class Vejler
{
public:
    Vejler();
    static void draw(QPainter *, const _dataPolyg &, const _dataPoints &);
};

#endif // VEJLER_H
