#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <complex>

#include "rendering_worker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

    void paintEvent(QPaintEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private slots:
    void update_output();
private:
    QImage draw_preview();

    void send_input();

    Ui::MainWindow *ui;
    double current_scale = 1. / 200.;
    int last_x, last_y;
    bool pressed = false;
    std::complex<double> current_center_offset = 0;
    rendering_worker worker;
    QImage image_to_render;
};
#endif // MAINWINDOW_H
