#pragma once

#include "renderers/Renderer.hpp"
#include <gtkmm.h>
#include <vector>
#include <mutex>
#include <map>
#include <deque>

namespace nbody {

// Область рисования
template <typename T>
class NBodyDrawingArea : public Gtk::DrawingArea {
public:
    NBodyDrawingArea() {
        add_events(
            Gdk::BUTTON_PRESS_MASK   |
            Gdk::BUTTON_RELEASE_MASK |
            Gdk::SCROLL_MASK         |
            Gdk::SMOOTH_SCROLL_MASK  |
            Gdk::BUTTON1_MOTION_MASK
        );

        m_trail_length = 200;
        
        m_base_colors = {
            {0.2, 0.4, 1.0},  // Синий
            {1.0, 0.5, 0.1},  // Оранжевый
            {1.0, 1.0, 1.0}   // Белый
        };
    }
    
    void set_system(const System<T>* system, const Renderer<T>* renderer) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_system = system;
        m_renderer = renderer;
        
        if (m_colors.empty() && m_system && !m_system->bodies().empty()) {
            m_colors.resize(m_system->bodies().size());
            for (size_t i = 0; i < m_colors.size(); ++i) {
                Gdk::RGBA color;
                
                if (i < m_base_colors.size()) {
                    color.set_rgba(std::get<0>(m_base_colors[i]), 
                                   std::get<1>(m_base_colors[i]), 
                                   std::get<2>(m_base_colors[i]), 
                                   1.0);
                } else {
                    double hue = i * 360.0 / m_colors.size();
                    color.set_hsv(hue, 0.9, 1.0);
                }
                
                m_colors[i] = color;
            }
        }
        
        queue_draw();
    }
    
    void add_trail_points() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_system || !m_renderer) return;
        
        const auto& bodies = m_system->bodies();
        if (m_trails.size() != bodies.size()) {
            m_trails.resize(bodies.size());
        }
        
        for (size_t i = 0; i < bodies.size(); ++i) {
            const auto& body = bodies[i];
            auto& trail = m_trails[i];

            trail.push_back(std::make_pair(body.position().x(), body.position().y()));

            if (trail.size() > m_trail_length) {
                trail.pop_front();
            }
        }
    }
    
    void clear_trails() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_trails.clear();
        queue_draw();
    }
    
    void set_trail_length(size_t length) {
        m_trail_length = length;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& trail : m_trails) {
            while (trail.size() > m_trail_length) {
                trail.pop_front();
            }
        }
    }
    
    // События мыши для управления просмотром (зум, перемещение)
    bool on_scroll_event(GdkEventScroll* event) override {
        if (!m_renderer) return false;
        
        auto renderer = const_cast<Renderer<T>*>(m_renderer);
        
        if (event->direction == GDK_SCROLL_UP || event->delta_y < 0) {
            renderer->set_scale(renderer->scale() * T{1.1});
        } else if (event->direction == GDK_SCROLL_DOWN || event->delta_y > 0) {
            renderer->set_scale(renderer->scale() / T{1.1});
        }
        
        queue_draw();
        return true;
    }
    
    // Перемещение (захват и перетаскивание)
    bool on_button_press_event(GdkEventButton* event) override {
        if (event->button == 1) { // Левая кнопка мыши
            m_dragging = true;
            m_drag_start_x = event->x;
            m_drag_start_y = event->y;
            m_drag_last_offset_x = m_renderer ? m_renderer->offset_x() : T{0};
            m_drag_last_offset_y = m_renderer ? m_renderer->offset_y() : T{0};
        }
        return true;
    }
    
    bool on_button_release_event(GdkEventButton* event) override {
        if (event->button == 1) {
            m_dragging = false;
        }
        return true;
    }
    
    bool on_motion_notify_event(GdkEventMotion* event) override {
        if (m_dragging && m_renderer) {
            auto renderer = const_cast<Renderer<T>*>(m_renderer);
            double dx = event->x - m_drag_start_x;
            double dy = event->y - m_drag_start_y;
            
            renderer->set_offset_x(m_drag_last_offset_x + dx / renderer->scale());
            renderer->set_offset_y(m_drag_last_offset_y + dy / renderer->scale());
            queue_draw();
        }
        return true;
    }
    
protected:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        Gtk::Allocation allocation = get_allocation();
        const int width = allocation.get_width();
        const int height = allocation.get_height();

        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->paint();
        
        if (!m_system || !m_renderer) return true;
        
        // Отрисовка траекторий
        for (size_t i = 0; i < m_trails.size(); ++i) {
            const auto& trail = m_trails[i];
            if (trail.empty()) continue;
            
            if (i < m_colors.size()) {
                cr->set_source_rgba(
                    m_colors[i].get_red(), 
                    m_colors[i].get_green(), 
                    m_colors[i].get_blue(), 
                    0.6
                );
            } else {
                cr->set_source_rgba(1.0, 1.0, 1.0, 0.6);
            }
            cr->set_line_width(1.5);
            
            // Градиент прозрачности
            bool first = true;
            double alpha_fade = 0.8 / std::min(trail.size(), (size_t)100);
            double current_alpha = 0.1;
            
            for (const auto& point : trail) {
                int x = m_renderer->to_screen_x(point.first);
                int y = m_renderer->to_screen_y(point.second);
                
                if (first) {
                    cr->move_to(x, y);
                    first = false;
                } else {
                    cr->line_to(x, y);
                }
                
                current_alpha = std::min(current_alpha + alpha_fade, 0.9);
            }
            
            cr->stroke();
        }
        
        // Отрисовка тел с эффектом свечения
        const auto& bodies = m_system->bodies();
        for (size_t i = 0; i < bodies.size(); ++i) {
            const auto& body = bodies[i];
            int x = m_renderer->to_screen_x(body.position().x());
            int y = m_renderer->to_screen_y(body.position().y());
            
            // Отрисовка эффекта свечения (размытый круг под точкой)
            if (i < m_colors.size()) {
                Cairo::RefPtr<Cairo::RadialGradient> gradient = 
                    Cairo::RadialGradient::create(x, y, 0, x, y, 15.0);
                
                gradient->add_color_stop_rgba(0, 
                    m_colors[i].get_red(), 
                    m_colors[i].get_green(), 
                    m_colors[i].get_blue(), 
                    0.8);
                gradient->add_color_stop_rgba(1, 
                    m_colors[i].get_red(), 
                    m_colors[i].get_green(), 
                    m_colors[i].get_blue(), 
                    0.0);
                
                cr->set_source(gradient);
                cr->arc(x, y, 15.0, 0.0, 2.0 * M_PI);
                cr->fill();
                
                cr->set_source_rgb(1.0, 1.0, 1.0); // Яркий белый центр
            } else {
                cr->set_source_rgb(1.0, 1.0, 1.0);
            }
            cr->arc(x, y, 4.0, 0.0, 2.0 * M_PI);
            cr->fill();
        }
        
        return true;
    }
    
private:
    const System<T>* m_system = nullptr;
    const Renderer<T>* m_renderer = nullptr;
    std::mutex m_mutex;
    
    // Хранение траекторий для каждого тела
    std::vector<std::deque<std::pair<T, T>>> m_trails;
    size_t m_trail_length;
    
    // Цвета для тел и траекторий
    std::vector<Gdk::RGBA> m_colors;
    std::vector<std::tuple<double, double, double>> m_base_colors;
    
    // Переменные для перетаскивания
    bool m_dragging = false;
    double m_drag_start_x = 0;
    double m_drag_start_y = 0;
    T m_drag_last_offset_x = T{0};
    T m_drag_last_offset_y = T{0};
};

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class GtkmmRenderer : public Renderer<T> {
public:
    GtkmmRenderer() {
        this->offset_x_ = T{6.5};
        this->offset_y_ = T{3.5};

        this->scale_ = T{100};
    }
    
    bool initialize() override {
        m_drawing_area = std::make_unique<NBodyDrawingArea<T>>();
        m_drawing_area->set_size_request(1024, 700);

        m_control_box = std::make_unique<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 5);

        m_reset_view_button = std::make_unique<Gtk::Button>("Сбросить вид");
        m_reset_view_button->signal_clicked().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_reset_view_clicked));
        m_control_box->pack_start(*m_reset_view_button, false, false, 5);

        m_clear_trails_button = std::make_unique<Gtk::Button>("Очистить траектории");
        m_clear_trails_button->signal_clicked().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_clear_trails_clicked));
        m_control_box->pack_start(*m_clear_trails_button, false, false, 5);

        m_trail_length_label = std::make_unique<Gtk::Label>("Длина отслеживания траектории:");
        m_control_box->pack_start(*m_trail_length_label, false, false, 5);
        
        m_trail_length_scale = std::make_unique<Gtk::Scale>(Gtk::ORIENTATION_HORIZONTAL);
        m_trail_length_scale->set_range(100, 1000);
        m_trail_length_scale->set_value(200);
        m_trail_length_scale->set_size_request(200, -1);
        m_trail_length_scale->signal_value_changed().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_trail_length_changed));
        m_control_box->pack_start(*m_trail_length_scale, false, false, 0);
        
        return true;
    }
    
    void render(const System<T>& system) override {
        if (!m_drawing_area) {
            std::cerr << "ERROR: рендерер не инициализирован" << std::endl;
            return;
        }
        
        m_drawing_area->set_system(&system, this);
        m_drawing_area->add_trail_points();
        m_drawing_area->queue_draw();
    }
    
    bool process_events() override {
        return m_drawing_area != nullptr;
    }
    
    void shutdown() override {
        return;
    }
    
    // Методы для доступа к виджетам
    Gtk::Widget* get_drawing_area() { return m_drawing_area.get(); }
    Gtk::Widget* get_control_box() { return m_control_box.get(); }
    
private:
    void on_reset_view_clicked() {
        this->offset_x_ = T{0.0};
        this->offset_y_ = T{0.0};
        this->scale_ = T{150};
        m_drawing_area->queue_draw();
    }
    
    void on_clear_trails_clicked() {
        m_drawing_area->clear_trails();
    }
    
    void on_trail_length_changed() {
        int length = static_cast<int>(m_trail_length_scale->get_value());
        m_drawing_area->set_trail_length(length);
    }
    
private:
    std::unique_ptr<NBodyDrawingArea<T>> m_drawing_area;
    std::unique_ptr<Gtk::Box> m_control_box;
    
    // Элементы управления
    std::unique_ptr<Gtk::Button> m_reset_view_button;
    std::unique_ptr<Gtk::Button> m_clear_trails_button;
    std::unique_ptr<Gtk::Label> m_trail_length_label;
    std::unique_ptr<Gtk::Scale> m_trail_length_scale;
};

} // namespace nbody 