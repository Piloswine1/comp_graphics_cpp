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
#include <functional>

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

    int optionDraw = -1;
    int optionFill = false;

    static uint const sizeCanvas = 500;
    static uint const RESX = 3000;
    static uint const RESY = 1900;

    QPoint _p;
    QImage screen;

    int    buffFrame[RESX][RESY];
    double buffZ[RESX][RESY];

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
        coord();
        coord(double _x, double _y, double _z):
            x(_x), y(_y), z(_z)
        {}
        coord(const QVector3D &point):
            x(point.x()),
            y(point.y()),
            z(point.z())
        {}
        QPointF toPointF() const {return {x, y};}
        bool operator==(const coord &a) const {return x==a.x && y==a.y && z==a.z;}
        coord operator-(const coord &a) const {return {x-a.x, y-a.y, z-a.z};}
        coord operator+(const coord &a) const {return {x+a.x, y+a.y, z+a.z};}
        coord operator*(float t) {return {t*x, t*y, t*z};}
    };

    enum COORD_STAT {};
    struct coord_weiler : public coord {
        COORD_STAT stat;
        bool inter;
        coord_weiler(const coord &c): coord(c) {};
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

    coord lightCoord = QVector3D{50, 300, 0};

    _dataPolyg dataPolygons;
    _dataPoints dataPoints;
    _polygonsF dataShapes;

    /* --------- Functions --------- */

    _dataPolyg parsePolygons(QFile *);
    _dataPoints parsePoints(QFile *);

    void defaultDrawFigure();
    void drawFigureZBuffer();
    void draw_fn() {
        optionDraw < 0 ? defaultDrawFigure() :
        optionDraw < 2 ? drawFigureZBuffer() :
        optionDraw < 4 ? drawFigureVeyler()  :
        throw std::range_error("wrong OptionDraw");
    };
    void drawFigureVeyler();
    void draw_reduced(const _polygonsF&);
    void draw_custom();

    void calculate(QMatrix4x4 &);

    void fillPolygon(int, QVector<intCoord>&);
    void customLine(int, intCoord&, intCoord&, QMap<int, QVector<intCoord>>&);
    void addInBuffFrame(int, int, int);

    bool points_covered(const coord&, const coord&);

    std::pair<bool, Frame::coord_weiler> add_path(const Frame::coord &c) {return {false, c};};

    _polygonsF prepare_polygons();

    _polygonsF reduce_polygons(_polygonsF);
    _polygonsF reduce_polygons_sub(const _onePolygonsF &, const _polygonsF &);
    std::pair<_polygonsF, _polygonsF> weiler_clip(const _onePolygonsF &, const _onePolygonsF &);

    std::pair<coord, coord> get_rect(const _onePolygonsF &);

    template<class T>
    std::pair<_polygonsF, _polygonsF> makePolygVeiler(T first, T second)
    {
        return fill(first.cbegin(), first.cend(), second.cbegin(), second.cend());
    }

    template<class It>
    std::pair<_polygonsF, _polygonsF> fill(It b1, It e1, It b2, It e2) {
        typedef typename std::iterator_traits<It>::value_type T;
        typedef QVector<T> vecT;

        auto find_left = [&](const T &val) {return std::find(b1, e1, val);};
        auto find_right = [&](const T &val) {return std::find(b2, e2, val);};

        auto walkGenerator = [&](const auto &gen, const auto &ptr, bool left_pos) {
            for (auto gen_val = gen(); gen_val.second; gen_val = gen()) {
                It it = gen_val.first;
                vecT tmp{*it};
                bool prevInter = it->inter;
                it = std::next(it);
                while (it != gen_val.first) {
                    tmp.push_back(*it);
                    if (it->inter) {
                        if (prevInter) {
                            it = (left_pos)? find_right(*it) : find_left(*it);
                            left_pos = !left_pos;
                        } else {
                            prevInter = true;
                            it = std::next(it);
                        }
                    } else {
                        it = std::next(it);
                    }
                }
                ptr->push_back(tmp);
            }
        };

        auto front = std::make_shared<QVector<vecT>>();
        auto genF = makeGenerator(b1, e1, front);
        walkGenerator(genF, front, true);

        auto back = std::make_shared<QVector<vecT>>();
        auto genB = makeGenerator(b2, e2, back);
        walkGenerator(genB, back, false);

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

    template<class It, class VecShared, class FnRet = std::pair<It, bool>, class RetVal = std::function<FnRet(void)>>
    RetVal static makeGenerator(It a, It b, VecShared vec) {
        RetVal genFn = [it = a, &a, &b, &vec, &genFn]() mutable -> FnRet {
            if (it == b)
                return {it, false};
            it = std::find_if(
                        (it == a)? it : std::next(it),
                        b,
                        [](const auto &val) { return val.inter; }
            );
            if (std::any_of(vec->begin(), vec->end(),
                            [&](const auto &subVec) { return std::find(subVec.begin(), subVec.end(), *it) != subVec.end(); })
                    && it != b)
                it = genFn().first;
            return {it, it != b};
        };
        return genFn;
    };
};

#endif // FRAME_H
