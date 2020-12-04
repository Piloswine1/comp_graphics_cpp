#ifndef GURO_H
#define GURO_H

#include "common.h"

class Guro
{
public:
    Guro();
    static void draw(QPainter *, const _dataPolyg &, const _dataPoints &);
};

#endif // GURO_H
