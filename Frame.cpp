#include "Frame.h"

Frame::Frame(QWidget *parent) : QWidget(parent)
{
    screen = QImage(sizeCanvas - 1, sizeCanvas - 1, QImage::Format_ARGB32);
    // Light pos
    lightCoord.x = 250;
    lightCoord.y = 250;
    lightCoord.z = 100;
}

/* ------------------ Draw Figure ------------------ */

void Frame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    painter.begin(this);
    QPen pen(Qt::black, 1);
    painter.setPen(pen);
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
//    _dataPolyg tmpdataPolygons;
//    _dataPoints tmpdataPoints;
//    reduce_polygons(&tmpdataPolygons, &tmpdataPoints);
//    draw_reduced(tmpdataPolygons, tmpdataPoints);
    drawFigureZBuffer();
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
                                screen.setPixelColor(key, i, QColor(90, 90, 90));
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

void Frame::reduce_polygons(_dataPolyg *, _dataPoints *)
{
    // sort polygons
    auto onePolygComp = [&](const int* const& a, const int* const& b) { return dataPoints[*a].z() < dataPoints[*b].z(); };
    auto polygSort = [&](const _dataOnePolyg &a, const _dataOnePolyg &b){
        const auto min_a = std::min(a.begin(), a.end(), onePolygComp);
        const auto min_b = std::min(b.begin(), b.end(), onePolygComp);
        return *min_a < *min_b;
    };

    _dataPolyg sorted(dataPolygons);
    std::sort(sorted.begin(), sorted.end(), polygSort);

    //
    const auto first = sorted.front();
    sorted.pop_front();

    _dataPolyg front,
               back;

    for (const auto &polyg : sorted)
        Q_UNUSED(polyg)
//        отсечь как то надо, бляя тут сложн
//        if (isFront)
//            front.push_back(polyg);
//        else
//            back.push_back(polyg);

    _dataPolyg toReduce;
    const auto max = std::max(first.begin(), first.end(), onePolygComp);
    for (const auto &polyg : back)
        for (const auto &point : polyg)
            if (dataPoints[point].z() < *max)
                toReduce.push_back(polyg);

    if (!toReduce.empty())
        QDebug(QtMsgType::QtFatalMsg)<<"Пока хз че с этим делать!";
}

void Frame::draw_reduced(const _dataPolyg &, const _dataPoints &)
{

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
