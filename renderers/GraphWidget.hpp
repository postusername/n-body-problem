#pragma once

#include <cmath>
#include <deque>
#include <mutex>
#include <vector>

#include <gtkmm.h>



namespace nbody {

template <typename T>
class GraphWidget : public Gtk::DrawingArea {
public:
    GraphWidget() : max_points_(100000), offset_x_(0.0), offset_y_(0.0), scale_x_(5.0), scale_y_(100.0) {
        set_draw_func(sigc::mem_fun(*this, &GraphWidget::on_draw));
        m_update_dispatcher.connect(sigc::mem_fun(*this, &GraphWidget::on_update_from_thread));
        set_expand(true);
        
        m_controller_click = Gtk::GestureClick::create();
        m_controller_click->set_button(GDK_BUTTON_PRIMARY);
        m_controller_click->signal_pressed().connect(
            sigc::mem_fun(*this, &GraphWidget::on_button_pressed));
        m_controller_click->signal_released().connect(
            sigc::mem_fun(*this, &GraphWidget::on_button_released));
        add_controller(m_controller_click);
        
        m_controller_scroll = Gtk::EventControllerScroll::create();
        m_controller_scroll->set_flags(
            Gtk::EventControllerScroll::Flags::VERTICAL | 
            Gtk::EventControllerScroll::Flags::HORIZONTAL |
            Gtk::EventControllerScroll::Flags::DISCRETE |
            Gtk::EventControllerScroll::Flags::KINETIC
        );
        m_controller_scroll->signal_scroll().connect(sigc::mem_fun(*this, &GraphWidget::on_scroll), true);
        add_controller(m_controller_scroll);
        
        m_controller_motion = Gtk::EventControllerMotion::create();
        m_controller_motion->signal_motion().connect(
            sigc::mem_fun(*this, &GraphWidget::on_motion));
        add_controller(m_controller_motion);
    }
    
    void add_point(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_points.empty()) {
            min_value_ = max_value_ = value;
        } else {
            min_value_ = std::min(min_value_, value);
            max_value_ = std::max(max_value_, value);
        }
        
        m_points.push_back(value);
        
        if (m_points.size() > max_points_) {
            m_points.pop_front();
            recalculate_minmax();
        }

        m_update_dispatcher.emit();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_points.clear();
        queue_draw();
    }
    
    void set_max_points(size_t max_points) {
        std::lock_guard<std::mutex> lock(m_mutex);
        max_points_ = max_points;
        while (m_points.size() > max_points_) {
            m_points.pop_front();
        }
        recalculate_minmax();
    }
    
    void reset_view() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_points.empty()) {
            T value_range = max_value_ - min_value_;
            
            if (value_range < T{1e-10}) {
                scale_y_ = 100.0;
                offset_y_ = -double(min_value_);
            } else {
                double range_double = double(value_range);
                scale_y_ = std::min(1000.0, std::max(10.0, 300.0 / range_double));
                offset_y_ = -double(min_value_ + max_value_) / 2.0;
            }
            
            if (m_points.size() > 0) {
                scale_x_ = std::max(1.0, std::min(10.0, 400.0 / double(m_points.size())));
            } else {
                scale_x_ = 5.0;
            }
            offset_x_ = 0.0;
        }
        auto_scale();
        queue_draw();
    }
    
    bool on_scroll(double, double dy) {
        double mouse_x = m_last_mouse_x;
        double mouse_y = m_last_mouse_y;
        
        int width = get_width();
        int height = get_height();
        const int margin = 50;
        
        double old_scale_x = scale_x_;
        double old_scale_y = scale_y_;
        
        double graph_x = (mouse_x - margin) / old_scale_x - offset_x_;
        double graph_y = (height - mouse_y - margin) / old_scale_y - offset_y_;
        
        bool scale_x_only = false;
        bool scale_y_only = false;
        
        if (mouse_x <= margin && mouse_y >= margin && mouse_y <= height - margin) {
            scale_y_only = true;
        } else if (mouse_y >= height - margin && mouse_x >= margin && mouse_x <= width - margin) {
            scale_x_only = true;
        }
        
        if (dy < 0) {
            if (!scale_y_only) scale_x_ *= 1.1;
            if (!scale_x_only) scale_y_ *= 1.1;
        } else if (dy > 0) {
            if (!scale_y_only) scale_x_ /= 1.1;
            if (!scale_x_only) scale_y_ /= 1.1;
        }
        
        if (!scale_y_only) {
            offset_x_ = -(graph_x - (mouse_x - margin) / scale_x_);
        }
        if (!scale_x_only) {
            offset_y_ = -(graph_y - (height - mouse_y - margin) / scale_y_);
        }
        
        queue_draw();
        return true;
    }
    
    void on_button_pressed(int, double x, double y) {
        m_dragging = true;
        m_drag_start_x = x;
        m_drag_start_y = y;
        m_drag_last_offset_x = offset_x_;
        m_drag_last_offset_y = offset_y_;
    }
    
    void on_button_released(int, double, double) {
        m_dragging = false;
    }
    
    void on_motion(double x, double y) {
        m_last_mouse_x = x;
        m_last_mouse_y = y;
        
        if (m_dragging) {
            double dx = x - m_drag_start_x;
            double dy = y - m_drag_start_y;
            
            offset_x_ = m_drag_last_offset_x + dx / scale_x_;
            offset_y_ = m_drag_last_offset_y - dy / scale_y_;
            queue_draw();
        }
    }
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->paint();
        
        const int margin = 50;
        int graph_width = width - 2 * margin;
        int graph_height = height - 2 * margin;
        
        if (graph_width <= 0 || graph_height <= 0) return;
        
        cr->set_source_rgb(0.2, 0.2, 0.2);
        cr->set_line_width(1.0);
        
        double x_min = -offset_x_;
        double x_max = graph_width / scale_x_ - offset_x_;
        double y_min = -offset_y_;
        double y_max = graph_height / scale_y_ - offset_y_;
        
        auto nice_number = [](double value, bool round) -> double {
            if (std::abs(value) < 1e-15) {
                return 1.0;
            }
            
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
        
        double x_range = x_max - x_min;
        double y_range = y_max - y_min;
        
        double x_tick_spacing = (x_range == 0) ? abs(x_min) : nice_number(x_range / 10, true);
        double y_tick_spacing = (y_range == 0) ? abs(y_min) : nice_number(y_range / 10, true);
        
        cr->set_source_rgb(0.3, 0.3, 0.3);
        cr->set_line_width(0.5);
        
        double x_start = std::floor(x_min / x_tick_spacing) * x_tick_spacing;
        for (double x = x_start; x <= x_max; x += x_tick_spacing) {
            int screen_x = margin + (x + offset_x_) * scale_x_;
            if (screen_x >= margin && screen_x <= width - margin) {
                cr->move_to(screen_x, margin);
                cr->line_to(screen_x, height - margin);
                cr->stroke();
            }
        }
        
        double y_start = std::floor(y_min / y_tick_spacing) * y_tick_spacing;
        for (double y = y_start; y <= y_max; y += y_tick_spacing) {
            int screen_y = height - margin - (y + offset_y_) * scale_y_;
            if (screen_y >= margin && screen_y <= height - margin) {
                cr->move_to(margin, screen_y);
                cr->line_to(width - margin, screen_y);
                cr->stroke();
            }
        }
        
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->set_line_width(2.0);
        
        int zero_x = margin + offset_x_ * scale_x_;
        int zero_y = height - margin - offset_y_ * scale_y_;
        
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
        
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->set_font_size(10);
        
        for (double x = x_start; x <= x_max; x += x_tick_spacing) {
            int screen_x = margin + (x + offset_x_) * scale_x_;
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
            int screen_y = height - margin - (y + offset_y_) * scale_y_;
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
        
        if (m_points.size() < 2) {
            return;
        }
        
        cr->save();
        cr->rectangle(margin, margin, graph_width, graph_height);
        cr->clip();
        
        cr->set_source_rgb(0.0, 1.0, 0.0);
        cr->set_line_width(2.0);
        
        // Определяем видимый диапазон точек
        double x_min_vis = -offset_x_;
        double x_max_vis = graph_width / scale_x_ - offset_x_;
        
        size_t start_idx = std::max(0.0, x_min_vis - 1);
        size_t end_idx = std::min(double(m_points.size() - 1), x_max_vis + 1);
        
        if (start_idx >= m_points.size() || end_idx < start_idx) {
            cr->restore();
            return;
        }
        
        // максимум одна точка на пиксель по X
        const size_t max_points = std::min(size_t(graph_width * 2), end_idx - start_idx + 1);
        size_t step = std::max(size_t(1), (end_idx - start_idx + 1) / max_points);
        
        bool first_point = true;
        int last_screen_x = INT_MIN;
        
        for (size_t i = start_idx; i <= end_idx; i += step) {
            double x = double(i);
            double y = double(m_points[i]);
            
            int screen_x = margin + (x + offset_x_) * scale_x_;
            int screen_y = height - margin - (y + offset_y_) * scale_y_;
            
            // Пропускаем точки, которые попадают на тот же пиксель по X
            if (screen_x == last_screen_x && !first_point) {
                continue;
            }
            last_screen_x = screen_x;
            
            if (first_point) {
                cr->move_to(screen_x, screen_y);
                first_point = false;
            } else {
                cr->line_to(screen_x, screen_y);
            }
        }
        
        // Обязательно рисуем последнюю точку, если она не была нарисована
        if (step > 1 && end_idx != start_idx + ((end_idx - start_idx) / step) * step) {
            double x = double(end_idx);
            double y = double(m_points[end_idx]);
            
            int screen_x = margin + (x + offset_x_) * scale_x_;
            int screen_y = height - margin - (y + offset_y_) * scale_y_;
            
            if (screen_x != last_screen_x) {
                cr->line_to(screen_x, screen_y);
            }
        }
        
        cr->stroke();
        cr->restore();
        
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->set_font_size(12);
        cr->move_to(10, 20);
        cr->show_text("Энергия системы");

        if (!m_points.empty() && max_value_ != min_value_) {
            T max_delta = max_value_ - min_value_;

            T sum = T{0.};
            for (const auto& point : m_points) sum += point;
            T average = sum / T(double(m_points.size()));
            
            double percentage = abs(average) > T{1e-15} ? 
                (double(max_delta) / double(abs(average))) * 100.0 : 0.0;
            
            char delta_text[64];
            if (abs(max_delta) < 1e-3) {
                snprintf(delta_text, sizeof(delta_text), "max_delta: %.2e (%.1f%%)", 
                         double(max_delta), percentage);
            } else {
                snprintf(delta_text, sizeof(delta_text), "max_delta: %.6g (%.1f%%)", 
                         double(max_delta), percentage);
            }
            
            Cairo::TextExtents extents;
            cr->get_text_extents(delta_text, extents);
            cr->move_to(width - extents.width - 10, height - 10);
            cr->show_text(delta_text);
        }
    }
    
    void on_update_from_thread() {
        queue_draw();
    }
    
    void recalculate_minmax() {
        if (m_points.empty()) {
            return;
        }
        
        min_value_ = max_value_ = m_points[0];
        for (const auto& point : m_points) {    
            min_value_ = std::min(min_value_, point);
            max_value_ = std::max(max_value_, point);
        }
    }
    
    void auto_scale() {
        if (m_points.empty()) return;
        
        int height = get_height();
        if (height <= 0) height = 400;
        
        const int margin = 50;
        int graph_height = height - 2 * margin;
        
        T value_range = max_value_ - min_value_;
        
        if (value_range < T{1e-15}) {
            scale_y_ = 100.0;
            offset_y_ = -double(min_value_);
        } else {
            scale_y_ = double(graph_height * 0.9) / double(value_range);
            offset_y_ = -double(min_value_ + value_range / T{2}) + double(graph_height) / (2.0 * scale_y_);
        }
        
        scale_x_ = 2.0;
        offset_x_ = 0.0;
    }

private:
    std::mutex m_mutex;
    std::deque<T> m_points;
    size_t max_points_;
    T min_value_{0};
    T max_value_{0};
    Glib::Dispatcher m_update_dispatcher;
    
    double offset_x_, offset_y_;
    double scale_x_, scale_y_;
    
    Glib::RefPtr<Gtk::GestureClick> m_controller_click;
    Glib::RefPtr<Gtk::EventControllerScroll> m_controller_scroll;
    Glib::RefPtr<Gtk::EventControllerMotion> m_controller_motion;
    
    bool m_dragging = false;
    double m_drag_start_x, m_drag_start_y, m_drag_last_offset_x, m_drag_last_offset_y;
    double m_last_mouse_x, m_last_mouse_y;
};

} // namespace nbody 