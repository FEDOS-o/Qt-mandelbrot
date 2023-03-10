#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPainter>


MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&worker, &rendering_worker::output_calculated, this, &MainWindow::update_output);
    image_to_render = draw_preview();
    send_input();
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
    auto x = event->position().x();
    auto y = event->position().y();
    std::complex<double> new_center_offset(
            (x - width() / 2 + current_center_offset.real()) * prev_scale / current_scale - x + width() / 2,
            (y - height() / 2 + current_center_offset.imag()) * prev_scale / current_scale - y + height() / 2
    );
    current_center_offset = new_center_offset;
    send_input();
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
    send_input();
    event->accept();
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    send_input();
}


QImage MainWindow::draw_preview()
{
    int batch_size = rendering_worker::BEGIN_BATCH_SIZE;
    QImage img(width(), height(), QImage::Format_RGB888);
    uchar* image_start = img.bits();
    for (int j=0; j < img.height(); j += batch_size) {
        for (int i = 0; i < img.width(); i += batch_size) {
            double val = rendering_worker::pixel_color(i, j, img.width(), img.height(), current_scale, current_center_offset);
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


void MainWindow::send_input()
{
    worker.set_input(input_params(width(), height(), current_scale, current_center_offset));
}


void MainWindow::update_output()
{
    std::optional<QImage> result = worker.get_output();
    if (result) {
        image_to_render = *result;
        update();
    }
}




