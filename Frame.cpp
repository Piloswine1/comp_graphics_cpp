#include "Frame.h"

Frame::Frame(QWidget *parent) : QWidget(parent)
{
    screen = QImage(sizeCanvas - 1, sizeCanvas - 1, QImage::Format_ARGB32);
}

/* ------------------ Draw Figure ------------------ */

void Frame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    painter.begin(this);
    painter.setPen({Qt::black, 2});
    switch(optionDraw)
    {
        case 0:
            defaultDrawFigure();
            break;
        case 1:
            drawFigureZBuffer();
            break;
        case 2:
            drawFigureVeyler();
            break;
    }
    painter.end();
}

void Frame::defaultDrawFigure()
{
    for (const auto &polygon: dataPolygons)
    {
        QVector<QPoint> points;
        auto paint = [&](const QPoint &a, const QPoint &b) {
            painter.drawLine(a, b);
            return b;
        };

        for (const auto &point: polygon)
            points.push_back(dataPoints[point].toPoint() + QPoint{250, 250});

        std::accumulate(std::next(points.begin()), points.end(),
                        points.front(),
                        paint);

        painter.drawLine(points.front(), points.back());
    }
}

void Frame::drawFigureZBuffer()
{
    screen.fill(QColor(Qt::white).rgb());
    for (uint x = 0; x < sizeCanvas; x++)
    {
        for (uint y = 0; y < sizeCanvas; y++)
        {
            buffFrame[x][y] = 0;
            buffZ[x][y]     = -1000;
        }
    }

    for (int i = 0; i < dataPolygons.size(); i++)
    {
        QVector<intCoord> points;
        for (const auto &point: dataPolygons[i])
        {
            const auto& point2d = dataPoints[point];
            points.push_back
            ({
                int(point2d.x() + 250 + 0.5),
                int(point2d.y() + 250 + 0.5),
                point2d.z()
            });
        }

        fillPolygon(i + 1, points);
    }

    painter.drawImage(1, 1, screen);
}

void Frame::drawFigureVeyler()
{
    const auto prepared = prepare_polygons();
    const auto toDraw = reduce_polygons(prepared);
    draw_reduced(toDraw);
//    drawFigureZBuffer();
}

void Frame::fillPolygon(int idSegment, QVector<intCoord> &points)
{
    QMap<int, QVector<intCoord>> boundMap;
    for (int i = 0; i < points.size() - 1; i++)
        customLine(idSegment, points[i], points[i + 1], boundMap);

    customLine(idSegment, points.last(), points[0], boundMap);

    int distance = sqrt(pow((lightCoord.x - 250), 2) + pow((lightCoord.y - 250), 2) + pow(lightCoord.z, 2));

    foreach (int key, boundMap.keys())
    {
       QVector<intCoord> value = boundMap.value(key);
        for (int i = value[0].y; i < value[1].y; i++)
        {
            if (buffFrame[key][i] != idSegment)
            {
                double tmp = value[0].z + double(value[1].z - value[0].z) * double(double(i - value[0].y) / double(value[1].y - value[0].y));

                if (tmp >= buffZ[key][i])
                {
                    buffFrame[key][i] = 0;
                    // Magic, Goru without Guro
                    int tmpDistance = sqrt(pow((lightCoord.x - key), 2) + pow((lightCoord.y - i), 2) + pow((lightCoord.z - tmp), 2));
                    double betweenDistance = distance - tmpDistance * 0.8;
                    int alpha = 255.0 * (1.0 - betweenDistance / 100.0) < 255 ? 255.0 * (1.0 - betweenDistance / 100.0) : 255;
                    alpha = alpha < 0 ? 0 : alpha;
                    switch(optionDraw)
                    {
                        case 1:
                            optionFill ?
                                screen.setPixelColor(key, i, QColor(155, 155, 155, alpha)):
                                screen.setPixelColor(key, i, 4294967295);
                            break;
                        case 2:
                            optionFill ?
                                screen.setPixelColor(key, i, QColor(0, 20, 255, alpha)):
                                screen.setPixelColor(key, i, 4294967295);
                            break;
                    }
                    buffZ[key][i] = tmp;
                }
            }
        }
    }
}

void Frame::customLine(int idSegment, intCoord &p1, intCoord &p2, QMap<int, QVector<intCoord>> &boundMap)
{
    const int deltaX = abs(p2.x - p1.x);
    const int deltaY = abs(p2.y - p1.y);

    const int signX = p1.x < p2.x ? 1 : -1;
    const int signY = p1.y < p2.y ? 1 : -1;

    int error = deltaX - deltaY;

    int x = p1.x,
        y = p1.y;

    double tmp;

    while(x != p2.x || y != p2.y)
    {
        if (p1.x == p2.x)
        {
            tmp = p1.z + double(p2.z - p1.z) * double(double(y - p1.y) / double(p2.y - p1.y));
        }else
        {
            tmp = p1.z + double(p2.z - p1.z) * double(double(x - p1.x) / double(p2.x - p1.x));
        }

        if (tmp >= buffZ[x][y])
        {
            buffFrame[x][y] = idSegment;
            screen.setPixelColor(x, y, 4278190080); // Black
            buffZ[x][y] = tmp;
        }

        if (boundMap.find(x) == boundMap.end())
        {
            intCoord boundCoord;
            boundCoord.y = y;
            boundCoord.z = tmp;
            boundMap.insert(x, {boundCoord, boundCoord});
        }else{
            if (boundMap[x][0].y > y)
            {
                boundMap[x][0].y = y;
                boundMap[x][0].z = tmp;
            }else if(boundMap[x][1].y < y)
            {
                boundMap[x][1].y = y;
                boundMap[x][1].z = tmp;
            }
        }

        //Logical draw
        const int error2 = error * 2;

        if(error2 > -deltaY)
        {
            error -= deltaY;
            x += signX;
        }
        if(error2 < deltaX)
        {
            error += deltaX;
            y += signY;
        }
    }
}

int Frame::orientation(const Frame::coord &p, const Frame::coord &q, const Frame::coord &r)
{
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);

    if (val == 0) return 0;  // colinear

    return (val > 0)? 1: 2; // clock or counterclock wise
}

bool Frame::onSegment(const Frame::coord &p, const Frame::coord &q, const Frame::coord &r)
{
    if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
        q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
       return true;

    return false;
}

bool Frame::doIntersect(const Frame::coord &p1, const Frame::coord &q1, const Frame::coord &p2, const Frame::coord &q2)
{
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    // General case
    if (o1 != o2 && o3 != o4)
        return true;

    // Special Cases
    // p1, q1 and p2 are colinear and p2 lies on segment p1q1
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;

    // p1, q1 and q2 are colinear and q2 lies on segment p1q1
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;

    // p2, q2 and p1 are colinear and p1 lies on segment p2q2
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;

     // p2, q2 and q1 are colinear and q1 lies on segment p2q2
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;

    return false; // Doesn't fall in any of the above cases
}

Frame::_polygonsF Frame::prepare_polygons()
{
    _polygonsF retval;
    for (const auto &polygon : dataPolygons) {
        QVector<Frame::coord> temp;
        for (const auto &point: polygon)
            temp.push_back({dataPoints[point]});
        retval.push_back(temp);
    }
    return retval;
}

Frame::_polygonsF Frame::reduce_polygons(Frame::_polygonsF polygons)
{
    // сортирууем палигончики
    auto polygonsFComp = [&](const auto &a, const auto &b) { return a.z < b.z; };
    auto polygSort = [&](const auto &a, const auto &b){
        const auto min_a = std::min_element(a.begin(), a.end(), polygonsFComp);
        const auto min_b = std::min_element(b.begin(), b.end(), polygonsFComp);
        return min_a->z < min_b->z;
    };

    std::sort(polygons.begin(), polygons.end(), polygSort);
    const auto first = polygons.last(); polygons.pop_back();

    auto test_line = [&](const auto &toTest) -> bool {
        // heeelp
        int times = 1;
        bool isDot = false;

        auto check = [&](const auto &a, const auto &b) {
            if (shootPos == a ||
                shootPos == b)
                return b;
            if (doIntersect(startPos, shootPos, a, b)){
                times += 1;
            }
            return b;
        };
        std::accumulate(std::next(toTest.cbegin()), toTest.cend(),
                        toTest.first(),
                        check);
        return times % 2;
    };

    _polygonsF front{first}, back;
    for (const auto &polygon : polygons)
        if (!test_line(polygon))
            front.push_back(polygon);
        else
            back.push_back(polygon);

    const auto closest_point_it = std::min_element(first.begin(), first.end(), polygonsFComp);
    const auto closer_at_back = std::find_if(back.begin(), back.end(), [&](const QVector<Frame::coord> &polyg) {
        const auto min = std::min_element(polyg.begin(), polyg.end(), polygonsFComp);
        return min->z < closest_point_it->z;
    });
    if (closer_at_back != back.end()){
        // на самом деле нужно типо работать с этим полигоном, но я хз
        qDebug()<<"Пока хз че с этим делать!\n"<<front.size();
//        for (auto &point: *closer_at_back)
//            point.clr = "red";
//        front.push_back(*closer_at_back);
    }

    return front;
}

void Frame::draw_reduced(const _polygonsF &shape)
{
    auto paint = [&](const auto &a, const auto &b) {
        painter.setPen({a.clr, 2});
        painter.drawLine(
            QPointF{a.x, a.y} + QPointF{250, 250},
            QPointF{b.x, b.y} + QPointF{250, 250}
        );
        return b;
    };
    for (const auto &polyg : shape) {
        std::accumulate(std::next(polyg.begin()), polyg.end(),
                        polyg.front(),
                        paint);
        paint(polyg.front(), polyg.back());
    }
}

/* ------------------ Figure operations------------------ */

void Frame::mouseMoveEvent(QMouseEvent *event)
{
    if (!_p.isNull())
    {
        if (abs(rotationX - (event->x() - int(sizeCanvas / 2))) > 5 && abs(rotationY - (event->y() - int(sizeCanvas / 2))) > 5)
        {
            if (rotationX > (event->x() - int(sizeCanvas / 2)))
            {
                if (rotationY > (event->y() - int(sizeCanvas / 2)))
                {
                    if ( FiX > 0) FiX = -FiX;
                    rotateX(false);
                    FiY = abs(FiY);
                    rotateY(false);
                }else
                {
                    FiX = abs(FiX);
                    rotateX(false);
                    FiY = abs(FiY);
                    rotateY(false);
                }
            }else
            {
                if (rotationY > (event->y() - int(sizeCanvas / 2)))
                {
                    if ( FiX > 0) FiX = -FiX;
                    rotateX(false);
                    if ( FiY > 0) FiY = -FiY;
                    rotateY(false);
                }else
                {
                    FiX = abs(FiX);
                    rotateX(false);
                    if ( FiY > 0) FiY = -FiY;
                    rotateY(false);
                }
            }
            rotationX = event->x() - int(sizeCanvas / 2);
            rotationY = event->y() - int(sizeCanvas / 2);
        }
    }
    repaint();
}

// Rotation

void Frame::rotateX(bool b_repaint)
{
    QMatrix4x4 R
    (
        1,         0,        0, 0,
        0,  cos(FiX), sin(FiX), 0,
        0, -sin(FiX), cos(FiX), 0,
        0,         0,        0, 1
    );

    calculate(R);
    if (b_repaint) repaint();
}

void Frame::rotateY(bool b_repaint)
{
    QMatrix4x4 R
    (
        cos(FiY), 0, -sin(FiY), 0,
               0, 1,         0, 0,
        sin(FiY), 0,  cos(FiY), 0,
               0, 0,         0, 1
    );

    calculate(R);
    if (b_repaint) repaint();
}

void Frame::rotateZ(bool b_repaint)
{
    QMatrix4x4 R
    (
         cos(FiZ), sin(FiZ), 0, 0,
        -sin(FiZ), cos(FiZ), 0, 0,
                0,        0, 1, 0,
                0,        0, 0, 1
    );

    calculate(R);
    if (b_repaint) repaint();
}

// Scale

void Frame::scaleX(double scaleValue)
{
    QMatrix4x4 R
    (
        scaleValue, 0, 0, 0,
                 0, 1, 0, 0,
                 0, 0, 1, 0,
                 0, 0, 0, 1
    );

    calculate(R);
    repaint();
}

void Frame::scaleY(double scaleValue)
{
    QMatrix4x4 R
    (
        1,          0, 0, 0,
        0, scaleValue, 0, 0,
        0,          0, 1, 0,
        0,          0, 0, 1
    );

    calculate(R);
    repaint();
}

void Frame::scaleZ(double scaleValue)
{
    QMatrix4x4 R
    (
        1, 0,          0, 0,
        0, 1,          0, 0,
        0, 0, scaleValue, 0,
        0, 0,          0, 1
    );

    calculate(R);
    repaint();
}

// Move

void Frame::moveToCoord(double posX, double posY, double posZ)
{
    QMatrix4x4 R
    (
        1, 0, 0, posX,
        0, 1, 0, posY,
        0, 0, 1, posZ,
        0, 0, 0, 1
    );

    calculate(R);
    repaint();
}

// Reflect

void Frame::reflectX()
{
    QMatrix4x4 R
    (
        1,  0, 0, 0,
        0, -1, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1
    );

    calculate(R);
    repaint();
}

void Frame::reflectY()
{
    QMatrix4x4 R
    (
        -1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0, 1
    );

    calculate(R);
    repaint();
}

void Frame::reflectZ()
{
    QMatrix4x4 R
    (
        1,  0,  0, 0,
        0,  1,  0, 0,
        0,  0, -1, 0,
        0,  0,  0, 1
    );

    calculate(R);
    repaint();
}

void Frame::calculate(QMatrix4x4 &R)
{

    for (int i = 0; i < dataPoints.size(); i++)
        dataPoints[i] = R * dataPoints[i];
}


/* ------------------ Upload data for figure ------------------ */

bool Frame::upload(QString path)
{
    dataPolygons.clear();
    dataPoints.clear();
    QFile file(path);

    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Предупреждение!", "Файл " + path + " не найден!");
        return false;
    }

    dataPoints = parsePoints(&file);
    dataPolygons = parsePolygons(&file);

    qDebug()<<"POINTS:\n"<<dataPoints;
    qDebug()<<"POLYGONS:\n"<<dataPolygons;

    file.close();
    repaint();
    return true;
}

_dataPoints Frame::parsePoints(QFile *file)
{
    _dataPoints retval;

    for (QString tmp = file->readLine(); !tmp.contains("POINTS"); tmp = file->readLine())
        if (file->atEnd())
            return {};

    for (QString tmp = file->readLine(); !tmp.contains("POLYGONS"); tmp = file->readLine()) {
        QStringList list = tmp.trimmed().split(QRegExp(" "));
        retval.push_back({
            100 * list.at(0).toFloat(),
            100 * list.at(1).toFloat(),
            100 * list.at(2).toFloat()
        });
        if (file->atEnd())
            return {};
    }

    return retval;
}

_dataPolyg Frame::parsePolygons(QFile *file)
{
    _dataPolyg retval;

    for (QString tmp = file->readLine(); true; tmp = file->readLine()) {
        QStringList list = tmp.trimmed().split(QRegExp(" "));
        QVector<int> tmpVec;
        std::for_each(std::next(list.begin()), list.end(),
                      [&](const QString &str) {tmpVec.push_back(str.toInt());});
        retval.push_back(tmpVec);
        if (file->atEnd())
            break;
    }

    return retval;
}
