#include "lab.h"

Lab::Lab()
{

}

void Lab::draw(QPainter *painter, const _dataPolyg &dataPolygons, const _dataPoints& dataPoints)
{

    //    for (int i = 0; i < dataPolygons.size(); i++)
    //    {
    //        QPolygon points;

    //        for (int j = 0; j < dataPolygons[i].size(); j++)
    //              points.push_back(dataPoints[dataPolygons[i][j]].toPoint());

    //        painter.drawPolygon(points);
    //    }
        for (const auto &polygon: dataPolygons)
        {
            QVector<QPoint> points;
            auto paint = [&](const QPoint &a, const QPoint &b) {
                painter->drawLine(a, b);
                return b;
            };

            for (const auto &point: polygon)
                points.push_back(dataPoints[point].toPoint() + QPoint{250, 250});

            std::accumulate(std::next(points.begin()), points.end(),
                            points.front(),
                            paint);

            painter->drawLine(points.front(), points.back());
        }
}
