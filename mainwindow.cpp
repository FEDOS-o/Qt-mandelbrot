#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPainter>
#include <thread>
#include <vector>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&worker, &rendering_worker::output_calculated, this, &MainWindow::update_output);
    image_to_render = draw_preview();
    worker.set_input(input_params(width(), height(), current_scale, current_center_offset, 32));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.drawImage(0, 0, image_to_render);
}


void MainWindow::wheelEvent(QWheelEvent *event) {
    double prev_scale = current_scale;
    if (event->angleDelta().y() > 0) {
        current_scale *= std::pow(0.9, event->angleDelta().y() / 240.);
    } else {
        current_scale *= std::pow(1.1, -event->angleDelta().y() / 240.);
    }
    int a = 1;
    if (event->angleDelta().y() <= 0) {
        a = -a;
    }
    auto x = event->globalPosition().x();
    auto y = event->globalPosition().y();
    double diff_w = x - width() / 2;
    double diff_h = y - height() / 2;

    std::complex<double> new_center_offset(
                current_center_offset.real() + a *  diff_w * prev_scale * 7,
                current_center_offset.imag() + -a * diff_h * prev_scale * 7);
    current_center_offset = new_center_offset;
    worker.set_input(input_params(width(), height(), current_scale, current_center_offset, 64));
   // update();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
            pressed = true;
            last_x = event->globalPosition().x();
            last_y = event->globalPosition().y();
            event->accept();
        } else {
            event->ignore();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pressed = false;
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!pressed) {
        event->ignore();
    }
    std::complex<double> new_center_offset(
    current_center_offset.real() - (event->globalPosition().x() - last_x),
    current_center_offset.imag() - (event->globalPosition().y() - last_y)
    );
    current_center_offset = new_center_offset;
    last_x = event->globalPosition().x();
    last_y = event->globalPosition().y();
    worker.set_input(input_params(width(), height(), current_scale, current_center_offset, 64));
  //  update();
    event->accept();
}

QImage MainWindow::draw_preview()
{

    int batch_size = 64;
    QImage img(width(), height(), QImage::Format_RGB888);
    uchar* image_start = img.bits();
    for (int j=0; j < img.height(); j += batch_size) {
        for (int i = 0; i < img.width(); i += batch_size) {
            double val = pixel_color(i, j, img.width(), img.height(), current_scale, current_center_offset);


            for (int jj = j; jj < std::min(j + batch_size, img.height()); jj++) {
                uchar* p = image_start + jj * img.bytesPerLine() + i * sizeof(uchar) * 3;
                for (int ii = i; ii < std::min(i + batch_size, img.width()); ii++) {
                    *p++ = static_cast<uchar>(val * 0xff);
                    *p++ = static_cast<uchar>(val * 0.3 * 0xff);
                    *p++ = 0;
                }
            }
        }
    }
    return img;
}

double MainWindow::pixel_color(int x, int y, int width, int height, double scale, std::complex<double> center_offset)
{
    std::complex<double> c(x - width / 2., y - height / 2.);
     c += center_offset;
    c *= scale;

    std::complex<double> z = 0;

    size_t const MAX_STEPS = 2000;
    size_t step = 0;
    for (;;) {
        if (z.real() * z.real() + z.imag() * z.imag() >= 4.)
            return (step % 51) / 50.;
        if (step == MAX_STEPS)
            return 0;
        z = z * z + c;
        step++;
    }
}


void MainWindow::update_output()
{
    std::optional<QImage> result = worker.get_output();
    if (result) {
        image_to_render = *result;
        update();
    }
}




