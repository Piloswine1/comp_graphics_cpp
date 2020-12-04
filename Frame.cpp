#include "Frame.h"

Frame::Frame(QWidget *parent) : QWidget(parent)
{
    QPen pen(Qt::black, 1, Qt::DashLine, Qt::SquareCap, Qt::RoundJoin);
    painter.setPen(pen);
}

/* ------------------ Draw Figure ------------------ */

void Frame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    painter.begin(this);
    drawFigure();
    painter.end();
}

void Frame::drawFigure()
{
    for (int i = 0; i < dataPolygons.size(); i++)
    {
        QPointF points[dataPolygons[i].size()];

        for (int j = 0; j < dataPolygons[i].size(); j++) {
            const auto& temp = dataPoints[dataPolygons[i][j]];
            const auto point = temp.toPointF();
            const auto dist = temp.distanceToPlane({100, 100, 100}, {0.5, 0.5, 0.5});
            points[j] = QPointF(point - QPointF{dist, dist});
        }

        painter.drawPolygon(points, dataPolygons[i].size());
    }
}

/* ------------------ Figure operations------------------ */

void Frame::mouseMoveEvent(QMouseEvent *event)
{
    if (!_p.isNull())
    {
        if (abs(rotationX - (event->x() - 250)) > 5 && abs(rotationY - (event->y() - 250)) > 5)
        {
            if (rotationX > (event->x() - 250))
            {
                if (rotationY > (event->y() - 250))
                {
                    FiX = abs(FiX);
                    rotateX();
                    if ( FiY > 0) FiY = -FiY;
                    rotateY();
                }else
                {
                    if ( FiX > 0) FiX = -FiX;
                    rotateX();
                    if ( FiY > 0) FiY = -FiY;
                    rotateY();

                }
            }else
            {
                if (rotationY > (event->y() - 250))
                {
                    FiX = abs(FiX);
                    rotateX();
                    FiY = abs(FiY);
                    rotateY();
                }else
                {
                    if ( FiX > 0) FiX = -FiX;
                    rotateX();
                    FiY = abs(FiY);
                    rotateY();

                }
            }
            rotationX = event->x() - 250;
            rotationY = event->y() - 250;
        }
    }
}

// Rotation
void Frame::rotateX()
{
    QMatrix4x4 R
    (
        1,         0,        0, 0,
        0,  cos(FiX), sin(FiX), 0,
        0, -sin(FiX), cos(FiX), 0,
        0,         0,        0, 1
    );

    calculate(R);
    repaint();
}

void Frame::rotateY()
{
    QMatrix4x4 R
    (
        cos(FiY), 0, -sin(FiY), 0,
               0, 1,         0, 0,
        sin(FiY), 0,  cos(FiY), 0,
               0, 0,         0, 1
    );

    calculate(R);
    repaint();
}



void Frame::rotateZ()
{
    QMatrix4x4 R
    (
         cos(FiZ), sin(FiZ), 0, 0,
        -sin(FiZ), cos(FiZ), 0, 0,
                0,        0, 1, 0,
                0,        0, 0, 1
    );

    calculate(R);
    repaint();
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

Frame::_dataPoints Frame::parsePoints(QFile *file)
{
    _dataPoints retval;

    for (QString tmp = file->readLine(); !tmp.contains("POINTS"); tmp = file->readLine())
        if (file->atEnd())
            return {};

    for (QString tmp = file->readLine(); !tmp.contains("POLYGONS"); tmp = file->readLine()) {
        QStringList list = tmp.trimmed().split(QRegExp(" "));
        retval.push_back({
            100 * list.at(0).toFloat() + 250.f,
            100 * list.at(1).toFloat() + 250.f,
            100 * list.at(2).toFloat() + 250.f
        });
        if (file->atEnd())
            return {};
    }

    return retval;
}

Frame::_dataPolyg Frame::parsePolygons(QFile *file)
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
