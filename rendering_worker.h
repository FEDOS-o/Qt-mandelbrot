#pragma once

#include <QObject>
#include <QImage>
#include <complex>
#include <optional>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <memory>


struct rendering_result {

    QImage img;
    std::atomic<int> threads_finished;

    rendering_result(int w, int h);
};

struct input_params {
    int w, h;
    double scale;
    std::complex<double> center_offset;
    int batch;
    std::shared_ptr<rendering_result> result;

    input_params(int w, int h, double scale, std::complex<double> center_offset, int batch);
};

class rendering_worker : public QObject
{
    Q_OBJECT
public:
    rendering_worker();
    ~rendering_worker();

    void set_input(std::optional<input_params> val);
    std::optional<QImage> get_output() const;

signals:
    void output_calculated();

protected:
    double pixel_color(int x, int y, int width, int height, double scale, std::complex<double> center_offset);
private:
    void thread_proc(int thread_num);
    void batch_calc(uint64_t last_input_version, input_params params, int thread_num);
    void store_result(std::optional<QImage> const& result);

private slots:
    void notify_output();

private:
    static int const THREADS_MAX = 4;
    static uint64_t const INPUT_VERSION_QUIT = 0;

    mutable std::mutex m;
    std::atomic<uint64_t> input_version;
    std::atomic<int> current_layer;

    std::atomic<int> threads_finished_batch;
    std::atomic<int> threads_canceled = THREADS_MAX - 1;

    QImage current_image;
    std::condition_variable input_changed;
    std::optional<input_params> input;
    std::optional<QImage> output;
    bool notify_output_queued = false;
    std::vector<std::thread> worker_threads = std::vector<std::thread>(THREADS_MAX - 1);
};

