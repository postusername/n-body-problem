#pragma once

#include <gtkmm.h>
#include <vector>
#include <complex>
#include <algorithm>
#include <cmath>

namespace nbody {

class GridVisualizationWidget : public Gtk::DrawingArea {
public:
    enum class DataType {
        DENSITY,
        POTENTIAL,
        FFT_IN,
        FFT_OUT_MAGNITUDE
    };

    GridVisualizationWidget() : data_type_(DataType::DENSITY), slice_z_(0) {
        set_draw_func(sigc::mem_fun(*this, &GridVisualizationWidget::on_draw));
        set_hexpand(true);
        set_vexpand(true);
    }

    void set_density_data(const std::vector<double>& data, int grid_size) {
        density_data_ = data;
        grid_size_ = grid_size;
        if (data_type_ == DataType::DENSITY) {
            queue_draw();
        }
    }

    void set_potential_data(const std::vector<double>& data, int grid_size) {
        potential_data_ = data;
        grid_size_ = grid_size;
        if (data_type_ == DataType::POTENTIAL) {
            queue_draw();
        }
    }

    void set_fft_in_data(const std::vector<double>& data, int grid_size) {
        fft_in_data_ = data;
        grid_size_ = grid_size;
        if (data_type_ == DataType::FFT_IN) {
            queue_draw();
        }
    }

    void set_fft_out_data(const std::vector<std::complex<double>>& data, int grid_size) {
        fft_out_data_ = data;
        grid_size_ = grid_size;
        if (data_type_ == DataType::FFT_OUT_MAGNITUDE) {
            queue_draw();
        }
    }

    void set_data_type(DataType type) {
        data_type_ = type;
        queue_draw();
    }

    void set_slice_z(int z) {
        slice_z_ = std::max(0, std::min(z, grid_size_ - 1));
        queue_draw();
    }

    int get_slice_z() const { return slice_z_; }
    int get_grid_size() const { return grid_size_; }

protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
        if (grid_size_ == 0) return;

        cr->save();
        
        // Очищаем фон
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->paint();

        // Вычисляем размер пикселя
        double pixel_width = double(width) / grid_size_;
        double pixel_height = double(height) / grid_size_;

        std::vector<double> slice_data = get_current_slice_data();
        if (slice_data.empty()) {
            cr->restore();
            return;
        }

        // Находим min/max для нормализации
        auto [min_it, max_it] = std::minmax_element(slice_data.begin(), slice_data.end());
        double min_val = *min_it;
        double max_val = *max_it;
        double range = max_val - min_val;

        if (range == 0.0) range = 1.0;

        // Рисуем сетку
        for (int j = 0; j < grid_size_; ++j) {
            for (int i = 0; i < grid_size_; ++i) {
                int idx = j * grid_size_ + i;
                double normalized_val = (slice_data[idx] - min_val) / range;
                
                // Цветовая схема: от синего (низкие значения) к красному (высокие)
                double r, g, b;
                if (normalized_val < 0.5) {
                    r = 0.0;
                    g = normalized_val * 2.0;
                    b = 1.0 - normalized_val * 2.0;
                } else {
                    r = (normalized_val - 0.5) * 2.0;
                    g = 1.0 - (normalized_val - 0.5) * 2.0;
                    b = 0.0;
                }

                cr->set_source_rgb(r, g, b);
                cr->rectangle(i * pixel_width, j * pixel_height, pixel_width, pixel_height);
                cr->fill();
            }
        }

        cr->restore();
    }

private:
    std::vector<double> get_current_slice_data() {
        if (grid_size_ == 0 || slice_z_ >= grid_size_) return {};

        std::vector<double> slice_data(grid_size_ * grid_size_);

        switch (data_type_) {
            case DataType::DENSITY:
                if (density_data_.size() != static_cast<size_t>(grid_size_ * grid_size_ * grid_size_)) return {};
                for (int j = 0; j < grid_size_; ++j) {
                    for (int i = 0; i < grid_size_; ++i) {
                        int src_idx = slice_z_ * grid_size_ * grid_size_ + j * grid_size_ + i;
                        int dst_idx = j * grid_size_ + i;
                        slice_data[dst_idx] = density_data_[src_idx];
                    }
                }
                break;

            case DataType::POTENTIAL:
                if (potential_data_.size() != static_cast<size_t>(grid_size_ * grid_size_ * grid_size_)) return {};
                for (int j = 0; j < grid_size_; ++j) {
                    for (int i = 0; i < grid_size_; ++i) {
                        int src_idx = slice_z_ * grid_size_ * grid_size_ + j * grid_size_ + i;
                        int dst_idx = j * grid_size_ + i;
                        slice_data[dst_idx] = potential_data_[src_idx];
                    }
                }
                break;

            case DataType::FFT_IN:
                if (fft_in_data_.size() != static_cast<size_t>(grid_size_ * grid_size_ * grid_size_)) return {};
                for (int j = 0; j < grid_size_; ++j) {
                    for (int i = 0; i < grid_size_; ++i) {
                        int src_idx = slice_z_ * grid_size_ * grid_size_ + j * grid_size_ + i;
                        int dst_idx = j * grid_size_ + i;
                        slice_data[dst_idx] = fft_in_data_[src_idx];
                    }
                }
                break;

            case DataType::FFT_OUT_MAGNITUDE:
                // FFT output имеет размер grid_size * grid_size * (grid_size/2 + 1)
                if (fft_out_data_.size() != static_cast<size_t>(grid_size_ * grid_size_ * (grid_size_/2 + 1))) return {};
                for (int j = 0; j < grid_size_; ++j) {
                    for (int i = 0; i < grid_size_; ++i) {
                        int dst_idx = j * grid_size_ + i;
                        if (i <= grid_size_/2 && slice_z_ < grid_size_) {
                            int src_idx = slice_z_ * grid_size_ * (grid_size_/2 + 1) + j * (grid_size_/2 + 1) + i;
                            slice_data[dst_idx] = std::abs(fft_out_data_[src_idx]);
                        } else {
                            slice_data[dst_idx] = 0.0;
                        }
                    }
                }
                break;
        }

        return slice_data;
    }

    DataType data_type_;
    int grid_size_ = 0;
    int slice_z_ = 0;
    
    std::vector<double> density_data_;
    std::vector<double> potential_data_;
    std::vector<double> fft_in_data_;
    std::vector<std::complex<double>> fft_out_data_;
};

} // namespace nbody 