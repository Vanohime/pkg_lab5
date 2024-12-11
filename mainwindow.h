#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QVector>
#include <QPointF>
#include <QGraphicsTextItem>
#include <QButtonGroup>
#include <QRadioButton>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadFile();
    void clipLines();

private:
    QGraphicsScene *scene;
    QGraphicsView *view;
    QPushButton *loadButton;
    QPushButton *clipButton;
    QRadioButton *rectClipButton;
    QRadioButton *polyClipButton;
    QVector<QLineF> lines;
    QRectF clipWindow;
    QPolygonF clipPolygon;
    QVector<QPointF> normals;

    void drawCoordinateSystem();
    void drawLines();
    void drawClipWindow();
    void drawClipPolygon();
    bool cohenSutherlandClip(QLineF &line);
    int computeCode(const QPointF &point);
    bool cyrusBeckClip(QLineF &line);
    void calculateNormals();
};

#endif
