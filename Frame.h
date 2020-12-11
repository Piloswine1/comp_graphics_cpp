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

typedef QVector<int> _dataOnePolyg;
typedef QVector<_dataOnePolyg> _dataPolyg;
typedef QVector<QVector3D> _dataPoints;

class Frame : public QWidget
{
    Q_OBJECT
public:
    Frame(QWidget * parent = 0);
    bool upload(QString);
    void setFiX(double initFiX){FiX = initFiX * M_PI / 180;};
    void setFiY(double initFiY){FiY = initFiY * M_PI / 180;};
    void setFiZ(double initFiZ){FiZ = initFiZ * M_PI / 180;};
    // Rotation
    void rotateX(bool b_repaint);
    void rotateY(bool b_repaint);
    void rotateZ(bool b_repaint);
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
    // Set option draw
    void setOptionDraw(int  value){ optionDraw = value; };
    void setOptionFill(bool value){ optionFill = value; };

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
        rotateZ(true);
    };


private:
    QPainter painter;

    int optionDraw = 0;
    int optionFill = false;

    static uint const sizeCanvas = 500;

    QPoint _p;
    QImage screen;

    int    buffFrame[sizeCanvas][sizeCanvas];
    double buffZ[sizeCanvas][sizeCanvas];

    double FiX = 0.07;
    double FiY = 0.07;
    double FiZ = 0.07;

    int rotationX;
    int rotationY;

    struct intCoord{
        int x, y;
        double z;
    };

    struct coord{
        double x, y, z;
        QColor clr = {"black"};
        coord(const QVector3D &point):
            x(point.x()),
            y(point.y()),
            z(point.z())
        {}
        bool operator==(const coord &a) const {return x==a.x && y==a.y && z==a.z;}
    };

    typedef QVector<QVector<coord>> _polygonsF;

    coord lightCoord = QVector3D{250, 250, 100};

    _dataPolyg dataPolygons;
    _dataPoints dataPoints;

    /* --------- Functions --------- */

    _dataPolyg parsePolygons(QFile *);
    _dataPoints parsePoints(QFile *);

    void defaultDrawFigure();
    void drawFigureZBuffer();
    void drawFigureVeyler();

    void calculate(QMatrix4x4 &);

    void fillPolygon(int, QVector<intCoord>&);
    void customLine(int, intCoord&, intCoord&, QMap<int, QVector<intCoord>>&);
    void addInBuffFrame(int, int, int);

    int orientation(const coord &p, const coord &q, const coord &r);
    bool onSegment(const coord &p, const coord &q, const coord &r);
    bool doIntersect(const coord &p1, const coord &q1, const coord &p2, const coord &q2);

    _polygonsF prepare_polygons();
    _polygonsF reduce_polygons(_polygonsF);
    void draw_reduced(const _polygonsF&);
};

#endif // FRAME_H
