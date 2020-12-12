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

#include <memory>

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
        coord(double _x, double _y, double _z):
            x(_x), y(_y), z(_z)
        {}
        coord(const QVector3D &point):
            x(point.x()),
            y(point.y()),
            z(point.z())
        {}
        bool operator==(const coord &a) const {return x==a.x && y==a.y && z==a.z;}
        coord operator-(const coord &a) const {return {x-a.x, y-a.y, z-a.z};}
        coord operator+(const coord &a) const {return {x+a.x, y+a.y, z+a.z};}
        coord operator*(float t) {return {t*x, t*y, t*z};}
    };

    enum COORD_STAT {};
    struct coord_weiler : public coord {
        COORD_STAT stat;
        bool inter;
        coord_weiler(const coord &c, COORD_STAT _stat, bool _inter):
            coord(c), stat(_stat), inter(_inter)
        {}
        bool operator==(const coord_weiler &a) const {
            return static_cast<coord>(*this) == static_cast<coord>(a) &&
                   stat == a.stat &&
                   inter == a.inter;
        }
    };

    typedef QVector<coord_weiler> _onePolygonsFWeil;
    typedef QVector<coord> _onePolygonsF;
    typedef QVector<_onePolygonsF> _polygonsF;

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

    _polygonsF prepare_polygons();
    _polygonsF reduce_polygons(_polygonsF);
    _polygonsF reduce_polygons_sub(const _onePolygonsF &, const _polygonsF &);
    std::pair<_polygonsF, _polygonsF> weiler_clip(const _onePolygonsF &, const _onePolygonsF &);

    template<class T>
    std::pair<_polygonsF, _polygonsF> makePolygVeiler(T first, T second)
    {
        return fill<T>(first.cbegin(), first.cend(), second.cbegin(), second.cend());
    }

    template<class It, class T>
    std::pair<_polygonsF, _polygonsF> fill(It b1, It e1, It b2, It e2) {
        auto walkGenerator = [&](auto first, bool firstList, const auto &vec) {
            T tmp{*first};
            bool foundIntersect = first->inter;
            auto it = std::next(first);
            while (it != first) {
                tmp.push_back(it);
                if (it->inter) {
                    if (foundIntersect)

                } else {
                    it = std::next(it);
                }
            }
            vec->push_back(tmp);
        };

        auto front = std::make_shared<QVector<T>>();
        auto genF = makeGenerator(b1, e1, front);
        for (auto first = genF(); first != e1; first = genF())
            walkGenerator(first);

        auto back = std::make_shared<QVector<T>>();
        auto genB = makeGenerator(b2, e2, back);
        for (auto first = genB(); first != e2; first = genB())
            walkGenerator(first);

        return {to_polygonsF(front), to_polygonsF(back)};
    };

    template<class VecShared>
    _polygonsF static to_polygonsF(VecShared pnt)
    {
        _polygonsF retval;
        for (auto it = pnt->begin(); it != pnt->end(); ++it){
            _onePolygonsF tmp;
            for (const auto &coord_w : *it)
                tmp.push_back(static_cast<coord>(coord_w));
            retval.push_back(tmp);
        }
        return retval;
    }

    template<class Gen, class It, class VecShared>
    Gen makeGenerator(It a, It b, VecShared vec) {
         It genFn = [it = a, &b, &vec, &genFn]() mutable {
            it = std::find_if(it, b, [](const auto &val) { return val.inter;});
            if (std::find(vec->begin(), vec->end(), *it) == vec->end() && it != b)
                it = genFn();
            return it;
        };
        return genFn;
    };

    void draw_reduced(const _polygonsF&);
};

#endif // FRAME_H
