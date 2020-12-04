#ifndef FRAME_H
#define FRAME_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QFile>
#include <QMessageBox>
#include <QtMath>
#include <QDebug>
#include <QMatrix4x4>
#include <QMatrix>
#include <QMouseEvent>
#include <QWheelEvent>

class Frame : public QWidget
{
    Q_OBJECT
public:
    Frame(QWidget * parent = 0);
    bool upload(QString);
    void drawFigure();
    void setFiX(double initFiX){FiX = initFiX * M_PI / 180;};
    void setFiY(double initFiY){FiY = initFiY * M_PI / 180;};
    void setFiZ(double initFiZ){FiZ = initFiZ * M_PI / 180;};
    // Rotation
    void rotateX();
    void rotateY();
    void rotateZ();
    // Scale
    void scaleX(double);
    void scaleY(double);
    void scaleZ(double);
    // Move
    void moveToCoord(double, double, double);
    // Reflect
    void reflectX();
    void reflectY();
    void reflectZ();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            _p = event->pos();
            rotationX = _p.x() - 250;
            rotationY = _p.y() - 250;
        }
    }
    void mouseMoveEvent(QMouseEvent *) override;

    void wheelEvent(QWheelEvent *event) override
    {
        if (event->angleDelta().y() > 0)
            FiZ += 4.f;
        else
            FiZ -= 4.f;
        rotateZ();
    };


private:
    typedef QVector<QVector<int>> _dataPolyg;
    typedef QVector<QVector3D> _dataPoints;
    QPainter painter;

    QPoint _p;

    double FiX = 0.07;
    double FiY = 0.07;
    double FiZ = 0.07;

    int rotationX;
    int rotationY;

    struct coord{
        float x, y, z;
    };

    _dataPolyg dataPolygons;
    _dataPoints dataPoints;

    /* --------- Functions --------- */
    _dataPolyg parsePolygons(QFile *);
    _dataPoints parsePoints(QFile *);
    void calculate(QMatrix4x4 &);
};

#endif // FRAME_H
