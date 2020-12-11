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

    template<class T>
    float static dot(const T &a, const T &b) { return std::fabs(a.x - b.y) + (b.x - a.y); }
    template<class T>
    float static distance(const T &a, const T &b) { return std::sqrt(std::pow((b.x - a.x), 2) - std::pow((b.y - a.y), 2)); }
    template<class T>
    float static length_squared(const T &a, const T &b) { return std::fabs(std::pow((b.x - a.x), 2) - std::pow((b.y - a.y), 2));}

    float minimum_distance(const coord &_v, const coord &_w, const coord &_p) {
      // Return minimum distance between line segment vw and point p
      const float l2 = length_squared(_v, _w);  // i.e. |_w-_v|^2 -  avoid a sqrt
      if (l2 == 0.0) return distance(_p, _v);   // _v == __w case
      // Consider the line extending the segment, parameterized as _v + t (_w - _v).
      // _we find projection of point p onto the line.
      // It falls _where t = [(p-_v) . (_w-_v)] / |_w-_v|^2
      // _we clamp t from [0,1] to handle points outside the segment _v_w.
      const float t = std::max(0.f, std::min(1.f, dot(_p - _v, _w - _v) / l2));
      const coord proj = _v + (_w - _v) *  t;  // Projection falls on the segment
      return distance(_p, proj);
    }

    template<class T>
    bool test_plygon(const T &first, const T &toTest) {
        const auto [v, w] = getRect(toTest);
        auto test_closest = [&, &_v = v, &_w = v](const auto &a, const auto &b) {
            return minimum_distance(_v, _w, a) < minimum_distance(_v, _w, b);
        };
        const auto startPos = *std::min_element(first.begin(), first.end(), test_closest);
        const auto shootPos = coord{v.x + w.x / 2.f, v.y + w.y / 2.f, 0};
        int times = 0;

        auto check = [&](const auto &a, const auto &b) {
            if (shootPos == a ||
                shootPos == b)
                return b;
            if (doIntersect(startPos, shootPos, a, b)) {
                times += 1;
            }
            return b;
        };
        std::accumulate(std::next(toTest.cbegin()), toTest.cend(),
                        toTest.first(),
                        check);
        return times % 2;
    };

    std::pair<coord, coord> getRect(const _onePolygonsF &polyg)
    {
        auto polygonsFCompx = [](const auto &a, const auto &b) { return a.x < b.x; };
        auto polygonsFCompy = [](const auto &a, const auto &b) { return a.y < b.y; };
        const auto [minx, maxx] = std::minmax_element(polyg.begin(), polyg.end(), polygonsFCompx);
        const auto [miny, maxy] = std::minmax_element(polyg.begin(), polyg.end(), polygonsFCompy);
        return {{minx->x, maxy->y, 0}, {maxx->x, miny->y, 0}};
    }

    bool doIntersect(const coord &, const coord &, const coord &, const coord &);

    _polygonsF prepare_polygons();
    _polygonsF reduce_polygons(_polygonsF);
    void draw_reduced(const _polygonsF&);
};

#endif // FRAME_H
