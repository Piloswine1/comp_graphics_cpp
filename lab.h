#ifndef LAB_H
#define LAB_H

#include "common.h"

class Lab
{
public:
    Lab();
    static void draw(QPainter *, const _dataPolyg &, const _dataPoints &);
};

#endif // LAB_H
