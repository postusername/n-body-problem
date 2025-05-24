#pragma once

#include <chrono>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "core/Body.hpp"
#include "core/Vector.hpp"
#include "renderers/CairoRenderer.hpp"
#include "renderers/RenderDialog.hpp"
#include "renderers/VideoRecorder.hpp"

#include <cairomm/cairomm.h>



namespace nbody {

template<typename T>
class RenderEngine {
public:
    RenderEngine(const RenderSettings& settings, int width, int height);
    
    template<typename SystemType, typename SimulatorType, typename RendererType>
    bool execute(SystemType& system, SimulatorType& simulator, RendererType& renderer);
    
private:
    void print_progress(int current_frame, int total_frames, 
                       std::chrono::steady_clock::time_point start_time);
    
    bool create_output_directory();
    
    template<typename SystemType, typename RendererType>
    std::vector<uint8_t> capture_frame(const SystemType& system, const RendererType& renderer, 
                                      int width, int height, bool depth_mode = false);
    
    std::vector<uint8_t> create_energy_graph_frame(const std::vector<T>& energy_history, 
                                                   int width, int height);
    
    RenderSettings settings_;
    int width_, height_;
    int total_frames_;
    std::vector<T> energy_history_;
    
    // Единый рендерер для основного и режима глубины
    CairoRenderer<T> main_cairo_renderer_;
    CairoRenderer<T> depth_cairo_renderer_;
};

template<typename T>
RenderEngine<T>::RenderEngine(const RenderSettings& settings, int width, int height)
    : settings_(settings), width_(width), height_(height) {

    total_frames_ = std::max(1, static_cast<int>(settings_.duration / settings_.dt));

    main_cairo_renderer_.set_depth_mode(false);
    depth_cairo_renderer_.set_depth_mode(true);
}

template<typename T>
bool RenderEngine<T>::create_output_directory() {
    try {
        std::filesystem::create_directories(settings_.output_path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Не удалось создать директорию " << settings_.output_path 
                  << ": " << e.what() << std::endl;
        return false;
    }
}

template<typename T>
template<typename SystemType, typename SimulatorType, typename RendererType>
bool RenderEngine<T>::execute(SystemType& system, SimulatorType& simulator, RendererType& renderer) {
    std::cout << "Начало рендера..." << std::endl;
    std::cout << "Длительность: " << settings_.duration << " сек" << std::endl;
    std::cout << "Временной шаг: " << settings_.dt << std::endl;
    std::cout << "Кадров: " << total_frames_ << std::endl;
    std::cout << "Разрешение: " << width_ << "x" << height_ << std::endl;
    std::cout << "Выходная папка: " << settings_.output_path << std::endl;
    std::cout << std::endl;

    if (!create_output_directory()) {
        return false;
    }

    std::unique_ptr<VideoRecorder> main_recorder;
    std::unique_ptr<VideoRecorder> depth_recorder;
    std::unique_ptr<VideoRecorder> energy_recorder;
    
    if (settings_.save_main) {
        main_recorder = std::make_unique<VideoRecorder>(
            settings_.output_path + "/simulation.mp4", width_, height_);
        if (!main_recorder->initialize()) {
            std::cerr << "ERROR: Не удалось инициализировать запись основного режима" << std::endl;
            return false;
        }
        std::cout << "Создан рекордер для основного режима: " << settings_.output_path << "/simulation.mp4" << std::endl;
    }
    
    if (settings_.save_depth) {
        depth_recorder = std::make_unique<VideoRecorder>(
            settings_.output_path + "/depth.mp4", width_, height_);
        if (!depth_recorder->initialize()) {
            std::cerr << "ERROR: Не удалось инициализировать запись режима глубины" << std::endl;
            return false;
        }
        std::cout << "Создан рекордер для режима глубины: " << settings_.output_path << "/depth.mp4" << std::endl;
    }
    
    if (settings_.save_energy) {
        energy_recorder = std::make_unique<VideoRecorder>(
            settings_.output_path + "/energy.mp4", width_, height_);
        if (!energy_recorder->initialize()) {
            std::cerr << "ERROR: Не удалось инициализировать запись графика энергии" << std::endl;
            return false;
        }
        std::cout << "Создан рекордер для графика энергии: " << settings_.output_path << "/energy.mp4" << std::endl;
    }

    simulator.set_dt(settings_.dt);
    auto start_time = std::chrono::steady_clock::now();
    
    for (int frame = 0; frame < total_frames_; ++frame) {
        if (!simulator.step()) {
            std::cerr << "ERROR: Ошибка в шаге симуляции на кадре " << frame << std::endl;
            return false;
        }

        if (!system.is_valid()) {
            std::cerr << "ERROR: Система стала некорректной на кадре " << frame << std::endl;
            return false;
        }

        if (settings_.save_energy) {
            energy_history_.push_back(system.graph_value());
        }

        if (settings_.save_main && main_recorder) {
            auto frame_data = capture_frame(system, renderer, width_, height_, false);
            if (!main_recorder->write_frame(frame_data)) {
                std::cerr << "ERROR: Ошибка записи основного кадра " << frame << std::endl;
                return false;
            }
        }
        
        if (settings_.save_depth && depth_recorder) {
            auto frame_data = capture_frame(system, renderer, width_, height_, true);
            if (!depth_recorder->write_frame(frame_data)) {
                std::cerr << "ERROR: Ошибка записи кадра глубины " << frame << std::endl;
                return false;
            }
        }
        
        if (settings_.save_energy && energy_recorder) {
            auto frame_data = create_energy_graph_frame(energy_history_, width_, height_);
            if (!energy_recorder->write_frame(frame_data)) {
                std::cerr << "ERROR: Ошибка записи кадра графика энергии " << frame << std::endl;
                return false;
            }
        }

        main_cairo_renderer_.add_trail_points(system.bodies());
        depth_cairo_renderer_.add_trail_points(system.bodies());

        print_progress(frame + 1, total_frames_, start_time);
    }
    
    std::cout << std::endl << "Завершение записи видеофайлов..." << std::endl;

    if (main_recorder) main_recorder->finalize();
    if (depth_recorder) depth_recorder->finalize();
    if (energy_recorder) energy_recorder->finalize();
    
    std::cout << "Рендер завершен успешно!" << std::endl;
    return true;
}

template<typename T>
void RenderEngine<T>::print_progress(int current_frame, int total_frames, 
                                    std::chrono::steady_clock::time_point start_time) {
    float progress = static_cast<float>(current_frame) / total_frames;
    int bar_width = 50;
    int filled = static_cast<int>(progress * bar_width);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
    
    int eta_seconds = 0;
    if (current_frame > 0) {
        float avg_time_per_frame = static_cast<float>(elapsed) / current_frame;
        eta_seconds = static_cast<int>((total_frames - current_frame) * avg_time_per_frame);
    }
    
    int eta_hours = eta_seconds / 3600;
    int eta_minutes = (eta_seconds % 3600) / 60;
    eta_seconds = eta_seconds % 60;
    
    std::cout << "\r[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) std::cout << "=";
        else if (i == filled) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100.0f) << "% "
              << "(" << current_frame << "/" << total_frames << ") "
              << "ETA: " << std::setfill('0') << std::setw(2) << eta_hours << ":"
              << std::setw(2) << eta_minutes << ":" << std::setw(2) << eta_seconds;
    std::cout.flush();
}

template<typename T>
template<typename SystemType, typename RendererType>
std::vector<uint8_t> RenderEngine<T>::capture_frame(const SystemType& system, const RendererType& renderer, 
                                                    int width, int height, bool depth_mode) {
    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
    auto cr = Cairo::Context::create(surface);

    if (depth_mode) {
        depth_cairo_renderer_.render(cr, system, renderer, width, height, true, width/2.0, height/2.0, true);
    } else {
        main_cairo_renderer_.render(cr, system, renderer, width, height, true, width/2.0, height/2.0, true);
    }

    surface->flush();
    unsigned char* data = surface->get_data();
    int stride = surface->get_stride();
    std::vector<uint8_t> frame_data(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int src_idx = y * stride + x * 4;
            int dst_idx = (y * width + x) * 4;
            
            // Cairo использует BGRA, нам нужно RGBA
            frame_data[dst_idx]     = data[src_idx + 2]; // R
            frame_data[dst_idx + 1] = data[src_idx + 1]; // G
            frame_data[dst_idx + 2] = data[src_idx];     // B
            frame_data[dst_idx + 3] = data[src_idx + 3]; // A
        }
    }

    return frame_data;
}

template<typename T>
std::vector<uint8_t> RenderEngine<T>::create_energy_graph_frame(const std::vector<T>& energy_history, 
                                                               int width, int height) {
    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
    auto cr = Cairo::Context::create(surface);

    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->paint();
    
    if (energy_history.size() < 2) {
        surface->flush();
        unsigned char* data = surface->get_data();
        int stride = surface->get_stride();
        
        std::vector<uint8_t> frame_data(width * height * 4);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int src_idx = y * stride + x * 4;
                int dst_idx = (y * width + x) * 4;
                
                frame_data[dst_idx]     = data[src_idx + 2]; // R
                frame_data[dst_idx + 1] = data[src_idx + 1]; // G
                frame_data[dst_idx + 2] = data[src_idx];     // B
                frame_data[dst_idx + 3] = data[src_idx + 3]; // A
            }
        }
        return frame_data;
    }

    const int margin = 50;
    int graph_width = width - 2 * margin;
    int graph_height = height - 2 * margin;
    
    if (graph_width <= 0 || graph_height <= 0) {
        std::vector<uint8_t> frame_data(width * height * 4, 0);
        for (size_t i = 3; i < frame_data.size(); i += 4) {
            frame_data[i] = 255; // Alpha
        }
        return frame_data;
    }

    T min_energy = energy_history[0];
    T max_energy = energy_history[0];
    for (const auto& energy : energy_history) {
        if (energy < min_energy) min_energy = energy;
        if (energy > max_energy) max_energy = energy;
    }
    
    T energy_range = max_energy - min_energy;
    if (energy_range == T{0}) energy_range = T{1};

    double scale_x = double(graph_width) / double(energy_history.size() - 1);
    double scale_y = double(graph_height * 0.9) / double(energy_range);
    double offset_x = 0.0;
    double offset_y = -double(min_energy + energy_range / T{2}) + double(graph_height) / (2.0 * scale_y);

    auto nice_number = [](double value, bool round) -> double {
        if (std::abs(value) < 1e-15) return 1.0;
        
        double exp = std::floor(std::log10(std::abs(value)));
        double f = std::abs(value) / std::pow(10, exp);
        double nf;
        if (round) {
            if (f < 1.5) nf = 1;
            else if (f < 3) nf = 2;
            else if (f < 7) nf = 5;
            else nf = 10;
        } else {
            if (f <= 1) nf = 1;
            else if (f <= 2) nf = 2;
            else if (f <= 5) nf = 5;
            else nf = 10;
        }
        return nf * std::pow(10, exp);
    };

    double x_min = -offset_x;
    double x_max = graph_width / scale_x - offset_x;
    double y_min = -offset_y;
    double y_max = graph_height / scale_y - offset_y;
    
    double x_range = x_max - x_min;
    double y_range = y_max - y_min;
    
    double x_tick_spacing = (x_range == 0) ? std::abs(x_min) : nice_number(x_range / 10, true);
    double y_tick_spacing = (y_range == 0) ? std::abs(y_min) : nice_number(y_range / 10, true);

    // Рисуем сетку
    cr->set_source_rgb(0.3, 0.3, 0.3);
    cr->set_line_width(0.5);
    
    double x_start = std::floor(x_min / x_tick_spacing) * x_tick_spacing;
    for (double x = x_start; x <= x_max; x += x_tick_spacing) {
        int screen_x = margin + (x + offset_x) * scale_x;
        if (screen_x >= margin && screen_x <= width - margin) {
            cr->move_to(screen_x, margin);
            cr->line_to(screen_x, height - margin);
            cr->stroke();
        }
    }
    
    double y_start = std::floor(y_min / y_tick_spacing) * y_tick_spacing;
    for (double y = y_start; y <= y_max; y += y_tick_spacing) {
        int screen_y = height - margin - (y + offset_y) * scale_y;
        if (screen_y >= margin && screen_y <= height - margin) {
            cr->move_to(margin, screen_y);
            cr->line_to(width - margin, screen_y);
            cr->stroke();
        }
    }

    // Рисуем оси
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_line_width(2.0);
    
    int zero_x = margin + offset_x * scale_x;
    int zero_y = height - margin - offset_y * scale_y;
    
    if (zero_x >= margin && zero_x <= width - margin) {
        cr->move_to(zero_x, margin);
        cr->line_to(zero_x, height - margin);
        cr->stroke();
    }
    
    if (zero_y >= margin && zero_y <= height - margin) {
        cr->move_to(margin, zero_y);
        cr->line_to(width - margin, zero_y);
        cr->stroke();
    }

    // Подписи осей
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_font_size(10);
    
    for (double x = x_start; x <= x_max; x += x_tick_spacing) {
        int screen_x = margin + (x + offset_x) * scale_x;
        if (screen_x >= margin && screen_x <= width - margin) {
            char label[32];
            if (std::abs(x) < 1e-3 && x != 0) {
                snprintf(label, sizeof(label), "%.1e", x);
            } else {
                snprintf(label, sizeof(label), "%.3g", x);
            }
            Cairo::TextExtents extents;
            cr->get_text_extents(label, extents);
            cr->move_to(screen_x - extents.width / 2, height - margin + 15);
            cr->show_text(label);
        }
    }
    
    for (double y = y_start; y <= y_max; y += y_tick_spacing) {
        int screen_y = height - margin - (y + offset_y) * scale_y;
        if (screen_y >= margin && screen_y <= height - margin) {
            char label[32];
            if (std::abs(y) < 1e-3 && y != 0) {
                snprintf(label, sizeof(label), "%.1e", y);
            } else {
                snprintf(label, sizeof(label), "%.6g", y);
            }
            Cairo::TextExtents extents;
            cr->get_text_extents(label, extents);
            cr->move_to(margin - extents.width - 5, screen_y + extents.height / 2);
            cr->show_text(label);
        }
    }

    // Ограничиваем область рисования графика
    cr->save();
    cr->rectangle(margin, margin, graph_width, graph_height);
    cr->clip();
    cr->set_source_rgb(0.0, 1.0, 0.0);
    cr->set_line_width(2.0);
    
    bool first_point = true;
    for (size_t i = 0; i < energy_history.size(); ++i) {
        double x = double(i);
        double y = double(energy_history[i]);
        
        int screen_x = margin + (x + offset_x) * scale_x;
        int screen_y = height - margin - (y + offset_y) * scale_y;
        
        if (first_point) {
            cr->move_to(screen_x, screen_y);
            first_point = false;
        } else {
            cr->line_to(screen_x, screen_y);
        }
    }
    
    cr->stroke();
    cr->restore();

    // Заголовок
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_font_size(12);
    cr->move_to(10, 20);
    cr->show_text("Энергия системы");

    // Статистика max_delta
    if (energy_range != T{0}) {
        T sum = T{0};
        for (const auto& point : energy_history) sum += point;
        T average = sum / T(double(energy_history.size()));
        
        double percentage = abs(average) > T{1e-15} ? 
            (double(energy_range) / double(abs(average))) * 100.0 : 0.0;
        
        char delta_text[64];
        if (abs(energy_range) < 1e-3) {
            snprintf(delta_text, sizeof(delta_text), "max_delta: %.2e (%.1f%%)", 
                     double(energy_range), percentage);
        } else {
            snprintf(delta_text, sizeof(delta_text), "max_delta: %.6g (%.1f%%)", 
                     double(energy_range), percentage);
        }
        
        Cairo::TextExtents extents;
        cr->get_text_extents(delta_text, extents);
        cr->move_to(width - extents.width - 10, height - 10);
        cr->show_text(delta_text);
    }

    // Конвертация в RGBA
    surface->flush();
    unsigned char* data = surface->get_data();
    int stride = surface->get_stride();
    
    std::vector<uint8_t> frame_data(width * height * 4);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int src_idx = y * stride + x * 4;
            int dst_idx = (y * width + x) * 4;
            
            frame_data[dst_idx]     = data[src_idx + 2]; // R
            frame_data[dst_idx + 1] = data[src_idx + 1]; // G
            frame_data[dst_idx + 2] = data[src_idx];     // B
            frame_data[dst_idx + 3] = data[src_idx + 3]; // A
        }
    }
    
    return frame_data;
}

} // namespace nbody 