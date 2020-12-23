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
    draw_fn();
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
    for (uint x = 0; x < RESX; x++)
    {
        for (uint y = 0; y < RESY; y++)
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
            if (!optionFill) {
                buffFrame[x][y] = idSegment;
                screen.setPixelColor(x, y, 4278190080); // Black
            }
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

bool Frame::points_covered(const Frame::coord &pointa, const Frame::coord &pointb)
{
    bool a_covered = false;
    bool b_covered = false;
    auto check_cover = [](const auto &point, const auto &a, const auto &b) {
        return  point.x > a.x && point.x < b.x &&
                point.y > b.y && point.y < a.y &&
                point.z > a.z;
    };
    for (const auto &polygon: dataShapes) {
        const auto [a, b] = get_rect(polygon);
        if (check_cover(pointa, a, b)) a_covered = true;
        if (check_cover(pointb, a, b)) b_covered = true;
        if (a_covered && b_covered) return true;
    }
    return false;
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
    auto polygSort = [](const auto &a, const auto &b){
        auto sumF = [](const auto &a, const auto &b) { return a + b.z; };
        const auto sum_a = std::accumulate(std::next(a.begin()), a.end(), a.front().z, sumF);
        const auto sum_b = std::accumulate(std::next(b.begin()), b.end(), b.front().z, sumF);
        return sum_a < sum_b;
    };

    std::sort(polygons.begin(), polygons.end(), polygSort);
    const auto first = polygons.last(); polygons.pop_back();
    return reduce_polygons_sub(first, polygons);
}

Frame::_polygonsF Frame::reduce_polygons_sub(const Frame::_onePolygonsF &first, const _polygonsF &polygons)
{
    auto polygonsFCompz = [&](const auto &a, const auto &b) { return a.z < b.z; };

    _polygonsF front, back;
    for (const auto &polygon : polygons) {
        const auto [_front, _back] = weiler_clip(first, polygon);
        front.append(_front);
        back.append(_back);
    }

    const auto farest_point_it = std::min_element(first.begin(), first.end(), polygonsFCompz);
    const auto closer_at_back = std::find_if(back.begin(), back.end(), [&](const QVector<Frame::coord> &polyg) {
        const auto min = std::max_element(polyg.begin(), polyg.end(), polygonsFCompz);
        return min->z < farest_point_it->z;
    });

    if (closer_at_back != back.end()){
        const auto new_first = *closer_at_back;
        back.erase(closer_at_back);
        return reduce_polygons_sub(new_first, front + back);
    } else {
        return front;
    }
}

void Frame::draw_reduced(const _polygonsF &shape)
{
    auto paint = [&](const auto &a, const auto &b) {
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

std::pair<Frame::_polygonsF, Frame::_polygonsF> Frame::weiler_clip(const Frame::_onePolygonsF &clipBy, const Frame::_onePolygonsF &toClip)
{
    _onePolygonsFWeil toFront,
                      toBack;
    bool isClipped = false;
    std::for_each(toClip.cbegin(), toClip.cend(), [&](const auto &elem) {
        const auto [flag, _elem] = add_path(elem);
        if (flag)
            toFront.push_back(_elem);
        else
            toBack.push_back(_elem);
        if (std::find_if(toFront.begin(), toFront.end(), [&](const auto &elem){Q_UNUSED(elem); return isClipped;}) != toFront.end())
            isClipped = true;
    });
    if (!isClipped)
        return {{clipBy, toClip}, {}};

    return makePolygVeiler(toFront, toBack);
}

std::pair<Frame::coord, Frame::coord> Frame::get_rect(const Frame::_onePolygonsF &vec)
{
    auto cmpx = [&](const auto a, const auto b) {return a.x < b.x;};
    auto cmpy = [&](const auto a, const auto b) {return a.y < b.y;};
    const auto [minx, maxx] = std::minmax_element(vec.begin(), vec.end(), cmpx);
    const auto [miny, maxy] = std::minmax_element(vec.begin(), vec.end(), cmpy);
    return {{minx->x, maxy->y, maxy->z}, {maxx->x, miny->y, miny->z}};
}

/* ------------------ Figure operations------------------ */

void Frame::mouseMoveEvent(QMouseEvent *event)
{
    if (!_p.isNull())
    {
        if (abs(rotationX - (event->x() - int(RESX / 2))) > 5 && abs(rotationY - (event->y() - int(RESY / 2))) > 5)
        {
            if (rotationX > (event->x() - int(RESX / 2)))
            {
                if (rotationY > (event->y() - int(RESY / 2)))
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
                if (rotationY > (event->y() - int(RESY / 2)))
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
            rotationX = event->x() - int(RESX / 2);
            rotationY = event->y() - int(RESY / 2);
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
    dataShapes = prepare_polygons();
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
    dataShapes = prepare_polygons();

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
