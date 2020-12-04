#ifndef COMMON_H
#define COMMON_H

#include <QVector>
#include <QVector3D>
#include <QPainter>

typedef QVector<QVector<int>> _dataPolyg;
typedef QVector<QVector3D> _dataPoints;
typedef std::function<void(QPainter *, const _dataPolyg &, const _dataPoints &)> drawFunc;

#endif // COMMON_H
