#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QPen>
#include <QHBoxLayout>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    setMinimumSize(800, 600);


    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);


    loadButton = new QPushButton("Загрузить файл", this);
    clipButton = new QPushButton("Отсечь отрезки", this);
    clipButton->setEnabled(false);


    rectClipButton = new QRadioButton("Отсечение прямоугольником", this);
    polyClipButton = new QRadioButton("Отсечение многоугольником", this);
    rectClipButton->setChecked(true);

    QButtonGroup *buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(rectClipButton);
    buttonGroup->addButton(polyClipButton);


    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *radioLayout = new QHBoxLayout();
    radioLayout->addWidget(rectClipButton);
    radioLayout->addWidget(polyClipButton);

    mainLayout->addWidget(view);
    mainLayout->addLayout(radioLayout);
    mainLayout->addWidget(loadButton);
    mainLayout->addWidget(clipButton);

    setCentralWidget(centralWidget);


    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadFile);
    connect(clipButton, &QPushButton::clicked, this, &MainWindow::clipLines);
}

MainWindow::~MainWindow()
{
}

double polarAngle(const QPointF& point, const QPointF& center) {
    return atan2(point.y() - center.y(), point.x() - center.x());
}
void MainWindow::loadFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "", "Текстовые файлы (*.txt)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    lines.clear();
    clipPolygon.clear();
    normals.clear();
    QList<QPointF> list;
    QTextStream in(&file);


    int n;
    in >> n;


    for (int i = 0; i < n; ++i) {
        double x1, y1, x2, y2;
        in >> x1 >> y1 >> x2 >> y2;
        lines.append(QLineF(x1, y1, x2, y2));
    }


    double xmin, ymin, xmax, ymax;
    in >> xmin >> ymin >> xmax >> ymax;
    clipWindow = QRectF(xmin, ymin, xmax - xmin, ymax - ymin);


    int polyVertices;
    in >> polyVertices;


    for (int i = 0; i < polyVertices; ++i) {
        double x, y;
        in >> x >> y;
        list.append(QPointF(x, y));
    }

    QPointF center;
    for (const auto& p : list) {
        center.setX(center.x() + p.x());
        center.setY(center.y() + p.y());
    }
    center.setX(center.x() / polyVertices);
    center.setY(center.y() / polyVertices);

    std::sort(list.begin(), list.end(),
         [center](const QPointF& a, const QPointF& b) {
        return polarAngle(a, center) > polarAngle(b, center);
         });

    clipPolygon = QPolygonF(list);
    qDebug() << clipPolygon.size();
    if (!clipPolygon.isEmpty()) {
        clipPolygon << clipPolygon.first();
        calculateNormals();
    }
    file.close();
    for (const auto& p : clipPolygon) {
        qDebug() << p.x() << ' ' << p.y() << '\n';
    }

    scene->clear();
    drawCoordinateSystem();
    if (rectClipButton->isChecked()) {
        drawClipWindow();
    } else {
        drawClipPolygon();
    }
    drawLines();

    clipButton->setEnabled(true);
}

void MainWindow::calculateNormals()
{
    normals.clear();
    for (int i = 0; i < clipPolygon.size() - 1; ++i) {
        QPointF edge = clipPolygon[i + 1] - clipPolygon[i];

        float n_len = sqrt(edge.y()*edge.y() + edge.x()*edge.x());
        normals.append(QPointF(-edge.y() / n_len, edge.x() / n_len));
    }
}

void MainWindow::drawClipPolygon()
{
    QPen pen(Qt::blue, 2);
    for (int i = 0; i < clipPolygon.size() - 1; ++i) {
        scene->addLine(QLineF(clipPolygon[i], clipPolygon[i + 1]), pen);
    }
}

/*bool MainWindow::cyrusBeckClip(QLineF &line)
{
    QPointF a = line.p1(), b = line.p2(), ab = b - a;
    double tEnter = 0.0;
    double tLeave = 1.0;
    qDebug() << clipPolygon.size();
    for (int i = 0; i < clipPolygon.size() - 1; ++i) {
        QPointF side = clipPolygon[i + 1] - clipPolygon[i];
        double p = QPointF::dotProduct(ab, side);
        double q = QPointF::dotProduct(clipPolygon[i] - a, side);
        qDebug() << p << ' ' << q;
        if(p == 0){
            if(q > 0) {
                return false;
            } else {
                if (i == clipPolygon.size() - 1)
                    return true;
            }
        }
        if(p < 0) {
            tLeave = qMin(q/p, tLeave);
        }
        if(p > 0){
            tEnter = qMax(q/p, tEnter);
        }
        if(tEnter > tLeave)
            return false;
    }
    QPointF d = line.p2() - line.p1();
    if (tEnter <= tLeave) {
        QPointF newP1 = line.p1() + tEnter * d;
        QPointF newP2 = line.p1() + tLeave * d;
        line.setP1(newP1);
        line.setP2(newP2);
        return true;
    }
    return false;
}*/

bool MainWindow::cyrusBeckClip(QLineF &line)
{
    QPointF d = line.p2() - line.p1();
    double tEnter = 0.0;
    double tLeave = 1.0;

    for (int i = 0; i < clipPolygon.size() - 1; ++i) {
        QPointF normal = normals[i];
        QPointF w = line.p1() - clipPolygon[i];

        double dn = QPointF::dotProduct(d, normal);
        double wn = QPointF::dotProduct(w, normal);

        if (qFuzzyCompare(dn, 0.0)) {
            if (wn < 0) return false;
            continue;
        }

        double t = -wn / dn;
        if (dn < 0) {
            tEnter = qMax(tEnter, t);
        } else {
            tLeave = qMin(tLeave, t);
        }

        if (tEnter > tLeave) return false;
    }

    if (tEnter <= tLeave) {
        QPointF newP1 = line.p1() + tEnter * d;
        QPointF newP2 = line.p1() + tLeave * d;
        line.setP1(newP1);
        line.setP2(newP2);
        return true;
    }

    return false;
}
void MainWindow::clipLines()
{
    scene->clear();
    drawCoordinateSystem();

    if (rectClipButton->isChecked()) {
        drawClipWindow();
    } else {
        drawClipPolygon();
    }

    QPen clippedPen(Qt::green, 2);

    for (QLineF line : lines) {
        bool clipped = false;
        if (rectClipButton->isChecked()) {
            clipped = cohenSutherlandClip(line);
        } else {
            clipped = cyrusBeckClip(line);

        }
        if (clipped) {
            scene->addLine(line, clippedPen);

        } else {

        }
    }
}

void MainWindow::drawCoordinateSystem()
{
    QPen pen(Qt::black, 1);


    scene->addLine(QLineF(-400, 0, 400, 0), pen);
    scene->addLine(QLineF(0, -300, 0, 300), pen);

    for (int i = -40; i <= 40; i++) {
        scene->addLine(QLineF(i * 10, -3, i * 10, 3), pen);
        scene->addLine(QLineF(-3, i * 10, 3, i * 10), pen);
    }
}

void MainWindow::drawClipWindow()
{
    QPen pen(Qt::blue, 2);
    scene->addRect(clipWindow, pen);
}

void MainWindow::drawLines()
{
    QPen pen(Qt::red, 2);
    for (const QLineF &line : lines) {
        scene->addLine(line, pen);
    }
}


const int INSIDE = 0;
const int LEFT = 1;
const int RIGHT = 2;
const int BOTTOM = 4;
const int TOP = 8;

int MainWindow::computeCode(const QPointF &point)
{
    int code = INSIDE;

    if (point.x() < clipWindow.left())
        code |= LEFT;
    else if (point.x() > clipWindow.right())
        code |= RIGHT;

    if (point.y() < clipWindow.top())
        code |= TOP;
    else if (point.y() > clipWindow.bottom())
        code |= BOTTOM;

    return code;
}

bool MainWindow::cohenSutherlandClip(QLineF &line)
{
    QPointF p1 = line.p1();
    QPointF p2 = line.p2();

    int code1 = computeCode(p1);
    int code2 = computeCode(p2);

    bool accept = false;

    while (true) {
        if (!(code1 | code2)) {
            accept = true;
            break;
        }
        else if (code1 & code2) {
            break;
        }
        else {
            int codeOut = code1 ? code1 : code2;
            QPointF intersection;

            if (codeOut & TOP) {
                intersection.setX(p1.x() + (p2.x() - p1.x()) * (clipWindow.top() - p1.y()) / (p2.y() - p1.y()));
                intersection.setY(clipWindow.top());
            }
            else if (codeOut & BOTTOM) {
                intersection.setX(p1.x() + (p2.x() - p1.x()) * (clipWindow.bottom() - p1.y()) / (p2.y() - p1.y()));
                intersection.setY(clipWindow.bottom());
            }
            else if (codeOut & RIGHT) {
                intersection.setY(p1.y() + (p2.y() - p1.y()) * (clipWindow.right() - p1.x()) / (p2.x() - p1.x()));
                intersection.setX(clipWindow.right());
            }
            else if (codeOut & LEFT) {
                intersection.setY(p1.y() + (p2.y() - p1.y()) * (clipWindow.left() - p1.x()) / (p2.x() - p1.x()));
                intersection.setX(clipWindow.left());
            }

            if (codeOut == code1) {
                p1 = intersection;
                code1 = computeCode(p1);
            }
            else {
                p2 = intersection;
                code2 = computeCode(p2);
            }
        }
    }

    if (accept) {
        line.setP1(p1);
        line.setP2(p2);
    }

    return accept;
}

