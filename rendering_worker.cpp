#include <iostream>
#include "rendering_worker.h"

input_params::input_params(int w, int h, double scale, std::complex<double> center_offset, int batch) : w(w), h(h), scale(scale), center_offset(center_offset), batch(batch)
{

}

rendering_result::rendering_result(int w, int h)
{
    threads_finished = 0;
    img = QImage(w, h, QImage::Format_RGB888);
}

rendering_worker::rendering_worker() : input_version(INPUT_VERSION_QUIT + 1)
{
    int i = 0;
    for (auto &t : worker_threads) {
        t = std::thread([this, i] {thread_proc(i);});
        i++;
    }
}

rendering_worker::~rendering_worker()
{
    input_version = INPUT_VERSION_QUIT;
    input_changed.notify_all();
    for (auto &t : worker_threads) {
        t.join();
    }
}

void rendering_worker::set_input(std::optional<input_params> val)
{
    {
        std::lock_guard lg(m);
        val->result = std::make_shared<rendering_result>(val->w, val->h);
        input = val;
        ++input_version;
    }
    input_changed.notify_all();
}

std::optional<QImage> rendering_worker::get_output() const
{
    {
        std::lock_guard lg(m);
        return output;
    }
}

void rendering_worker::thread_proc(int thread_num)
{
    uint64_t last_input_version = 0;
    for (;;) {
        std::optional<input_params> input_copy;
        {
            std::unique_lock lg(m);
            input_changed.wait(lg, [&]{return input_version != last_input_version;});

            last_input_version = input_version;
            if (last_input_version == INPUT_VERSION_QUIT) {
                break;
            }

            input_copy = input;
        }

        std::optional<QImage> result;
        if (input_copy) {
            batch_calc(last_input_version, *input_copy, thread_num);
        } else {
            store_result(std::nullopt);
        }
    }
}

double rendering_worker::pixel_color(int x, int y, int width, int height, double scale, std::complex<double> center_offset)
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


void rendering_worker::batch_calc(uint64_t last_input_version, input_params params, int thread_num)
{
    if (last_input_version != input_version) return;
    std::shared_ptr<rendering_result> res = params.result;
    int layers_count = (params.h + params.batch - 1) / params.batch;
    int st = layers_count * thread_num / (THREADS_MAX - 1);
    int end = layers_count * (thread_num + 1) / (THREADS_MAX - 1);
    if (last_input_version != input_version) return;
    for (int layer = st; layer < end; layer++) {
        uchar* image_start = res->img.bits();
        int j = layer * params.batch;
        if (last_input_version != input_version) return;
        for (int i = 0; i < params.w; i += params.batch) {
            if (last_input_version != input_version) return;
            double val = pixel_color(i, j, params.w, params.h, params.scale, params.center_offset);
            if (last_input_version != input_version) return;
            for (int jj = j; jj < std::min(j + params.batch, params.h); jj++) {
                if (last_input_version != input_version) return;
                uchar* p = image_start + jj * res->img.bytesPerLine() + i * sizeof(uchar) * 3;
                for (int ii = i; ii < std::min(i + params.batch, params.w); ii++) {
                    if (last_input_version != input_version) return;
                    *p++ = static_cast<uchar>(val * 0xff);
                    *p++ = static_cast<uchar>(val * 0.3 * 0xff);
                    *p++ = 0;
                }
            }
        }
    }
    res->threads_finished++;
    if (res->threads_finished == THREADS_MAX - 1) {
        store_result(res->img);
        if (params.batch != 1) {
            set_input(input_params(params.w, params.h, params.scale, params.center_offset, params.batch / 2));
        }
    }
}




void rendering_worker::store_result(const std::optional<QImage> &result)
{
    std::lock_guard lg(m);
    output = result;

    if (!notify_output_queued) {
        QMetaObject::invokeMethod(this, "notify_output");
        notify_output_queued = true;
    }
}

void rendering_worker::notify_output()
{
    {
        std::lock_guard lg(m);
        notify_output_queued = false;
    }
    emit output_calculated();
}


