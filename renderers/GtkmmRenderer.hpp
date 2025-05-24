#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

#include "renderers/CairoRenderer.hpp"
#include "renderers/GraphWidget.hpp"
#include "renderers/RenderDialog.hpp"
#include "renderers/Renderer.hpp"

#include <gtkmm.h>



namespace nbody {

// Область рисования
template <typename T>
class NBodyDrawingArea : public Gtk::DrawingArea {
public:
    NBodyDrawingArea() {
        m_controller_click = Gtk::GestureClick::create();
        m_controller_click->set_button(GDK_BUTTON_PRIMARY);
        m_controller_click->signal_pressed().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_button_pressed));
        m_controller_click->signal_released().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_button_released));
        add_controller(m_controller_click);
        
        m_controller_middle_click = Gtk::GestureClick::create();
        m_controller_middle_click->set_button(GDK_BUTTON_MIDDLE);
        m_controller_middle_click->signal_pressed().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_middle_button_pressed));
        m_controller_middle_click->signal_released().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_middle_button_released));
        add_controller(m_controller_middle_click);
        
        m_controller_right_click = Gtk::GestureClick::create();
        m_controller_right_click->set_button(GDK_BUTTON_SECONDARY);
        m_controller_right_click->signal_pressed().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_right_button_pressed));
        m_controller_right_click->signal_released().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_right_button_released));
        add_controller(m_controller_right_click);
        
        m_controller_scroll = Gtk::EventControllerScroll::create();
        m_controller_scroll->set_flags(
            Gtk::EventControllerScroll::Flags::VERTICAL | 
            Gtk::EventControllerScroll::Flags::HORIZONTAL |
            Gtk::EventControllerScroll::Flags::DISCRETE |
            Gtk::EventControllerScroll::Flags::KINETIC
        );
        m_controller_scroll->signal_scroll().connect(sigc::mem_fun(*this, &NBodyDrawingArea::on_scroll), true);
        add_controller(m_controller_scroll);
        
        m_controller_motion = Gtk::EventControllerMotion::create();
        m_controller_motion->signal_motion().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_motion));
        m_controller_motion->signal_enter().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_mouse_enter));
        m_controller_motion->signal_leave().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_mouse_leave));
        add_controller(m_controller_motion);

        m_key_controller = Gtk::EventControllerKey::create();
        m_key_controller->signal_key_pressed().connect(
            sigc::mem_fun(*this, &NBodyDrawingArea::on_key_pressed), false);
        add_controller(m_key_controller);
        
        set_can_focus(true);
        set_focusable(true);

        set_draw_func(sigc::mem_fun(*this, &NBodyDrawingArea::on_draw));
        m_update_dispatcher.connect(sigc::mem_fun(*this, &NBodyDrawingArea::on_update_from_thread));
    }
    
    void set_system(const System<T>* system, const Renderer<T>* renderer) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_system = system;
        m_renderer = renderer;
        m_update_dispatcher.emit();
    }
    
    void add_trail_points() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_system) return;
        
        m_cairo_renderer.add_trail_points(m_system->bodies());
        m_update_dispatcher.emit();
    }
    
    void on_update_from_thread() {
        queue_draw();
    }
    
    void request_update() {
        m_update_dispatcher.emit();
    }
    
    void clear_trails() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cairo_renderer.clear_trails();
        queue_draw();
    }
    
    void set_trail_length(size_t length) {
        m_cairo_renderer.set_trail_length(length);
    }
    
    Vector<T> get_center_of_positions() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_system || m_system->bodies().empty()) {
            return Vector<T>(T{0}, T{0}, T{0});
        }

        Vector<T> center_of_positions(T{0}, T{0}, T{0});
        for (const auto& body : m_system->bodies())
            center_of_positions = center_of_positions + body.position();
        center_of_positions /= T{double(m_system->bodies().size())};
        
        return center_of_positions;
    }
    
    void get_bounds(T& min_x, T& max_x, T& min_y, T& max_y) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_system || m_system->bodies().empty()) {
            min_x = max_x = min_y = max_y = T{0};
            return;
        }
        
        const auto& first = m_system->bodies()[0];
        min_x = max_x = first.position().x();
        min_y = max_y = first.position().y();
        
        for (const auto& body : m_system->bodies()) {
            T x = body.position().x();
            T y = body.position().y();
            
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
    }
    
    // События мыши для управления просмотром (зум, перемещение)
    bool on_scroll(double, double dy) {
        if (!m_renderer) return false;   
        auto renderer = const_cast<Renderer<T>*>(m_renderer);
        
        auto controller = m_controller_motion;
        double mouse_x = m_last_mouse_x;
        double mouse_y = m_last_mouse_y;

        T old_scale = renderer->scale();
        T model_x = (mouse_x / old_scale) - renderer->offset_x();
        T model_y = (mouse_y / old_scale) - renderer->offset_y();
        
        T new_scale = old_scale;
        if (dy < 0) {
            new_scale = old_scale * T{1.1};
        } else if (dy > 0) {
            new_scale = old_scale / T{1.1};
        }
        
        renderer->set_scale(new_scale);
        T new_offset_x = -(model_x - (mouse_x / new_scale));
        T new_offset_y = -(model_y - (mouse_y / new_scale));
        renderer->set_offset_x(new_offset_x);
        renderer->set_offset_y(new_offset_y);
        
        queue_draw();
        return true;
    }
    
    // Перемещение (захват и перетаскивание)
    void on_button_pressed(int, double x, double y) {
        grab_focus();
        m_dragging = true;
        m_drag_start_x = x;
        m_drag_start_y = y;
        m_drag_last_offset_x = m_renderer ? m_renderer->offset_x() : T{0};
        m_drag_last_offset_y = m_renderer ? m_renderer->offset_y() : T{0};
    }
    
    void on_button_released(int, double, double) {
        m_dragging = false;
    }
    
    // Поворот (колёсико мыши)
    void on_middle_button_pressed(int, double x, double y) {
        m_rotating_y = true;
        m_rotate_start_x = x;
        m_rotate_start_y = y;
        m_rotate_last_angle_y = m_renderer ? m_renderer->rotation_y() : T{0};
    }
    
    void on_middle_button_released(int, double, double) {
        m_rotating_y = false;
    }
    
    // Поворот (пкм)
    void on_right_button_pressed(int, double x, double y) {
        m_rotating_x = true;
        m_rotate_start_x = x;
        m_rotate_start_y = y;
        m_rotate_last_angle_x = m_renderer ? m_renderer->rotation_x() : T{0};
    }
    
    void on_right_button_released(int, double, double) {
        m_rotating_x = false;
    }
    
    void on_motion(double x, double y) {
        m_last_mouse_x = x;
        m_last_mouse_y = y;
        
        if (m_dragging && m_renderer) {
            auto renderer = const_cast<Renderer<T>*>(m_renderer);
            double dx = x - m_drag_start_x;
            double dy = y - m_drag_start_y;
            
            renderer->set_offset_x(m_drag_last_offset_x + dx / renderer->scale());
            renderer->set_offset_y(m_drag_last_offset_y + dy / renderer->scale());
        }
        
        if (m_rotating_y && m_renderer) {
            auto renderer = const_cast<Renderer<T>*>(m_renderer);
            double dx = -(x - m_rotate_start_x);
            T rotation_delta = T(dx * 0.005);
            renderer->set_rotation_y(m_rotate_last_angle_y + rotation_delta);
        }
        
        if (m_rotating_x && m_renderer) {
            auto renderer = const_cast<Renderer<T>*>(m_renderer);
            double dy = y - m_rotate_start_y;
            T rotation_delta = T(dy * 0.005);
            renderer->set_rotation_x(m_rotate_last_angle_x + rotation_delta);
        }

        queue_draw();
    }
    
    void on_mouse_enter(double x, double y) {
        grab_focus();
        m_mouse_in_area = true;
        m_last_mouse_x = x;
        m_last_mouse_y = y;
        queue_draw();
    }
    
    void on_mouse_leave() {
        m_mouse_in_area = false;
        queue_draw();
    }
    
    // Обработчик клавиш
    bool on_key_pressed(guint keyval, guint, Gdk::ModifierType) {
        if (keyval == GDK_KEY_F1) {
            m_cairo_renderer.set_depth_mode(!m_cairo_renderer.get_depth_mode());
            queue_draw();
            return true;
        }
        return false;
    }
    
    CairoRenderer<T>& get_cairo_renderer() { return m_cairo_renderer; }
    
    const System<T>* get_system() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_system; 
    }
    
    const Renderer<T>* get_renderer() const { 
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_renderer; 
    }

protected:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_system || !m_renderer) {
            cr->set_source_rgb(0.0, 0.0, 0.0);
            cr->paint();
            return;
        }
        
        m_cairo_renderer.render(cr, *m_system, *m_renderer, width, height, 
                               true, m_last_mouse_x, m_last_mouse_y, m_mouse_in_area);
    }
    
private:
    const System<T>* m_system = nullptr;
    const Renderer<T>* m_renderer = nullptr;
    mutable std::mutex m_mutex;
    Glib::Dispatcher m_update_dispatcher;
    
    CairoRenderer<T> m_cairo_renderer;
    
    // Контроллеры для обработки событий мыши
    Glib::RefPtr<Gtk::GestureClick> m_controller_click;
    Glib::RefPtr<Gtk::GestureClick> m_controller_middle_click;
    Glib::RefPtr<Gtk::GestureClick> m_controller_right_click;
    Glib::RefPtr<Gtk::EventControllerScroll> m_controller_scroll;
    Glib::RefPtr<Gtk::EventControllerMotion> m_controller_motion;
    
    // Переменные для перетаскивания
    bool m_dragging = false;
    double m_drag_start_x = 0;
    double m_drag_start_y = 0;
    double m_last_mouse_x = 0;
    double m_last_mouse_y = 0;
    T m_drag_last_offset_x = T{0};
    T m_drag_last_offset_y = T{0};
    bool m_mouse_in_area = false;
    
    // Переменные для поворота
    bool m_rotating_x = false;
    bool m_rotating_y = false;
    double m_rotate_start_x = 0;
    double m_rotate_start_y = 0;
    T m_rotate_last_angle_x = T{0};
    T m_rotate_last_angle_y = T{0};

    Glib::RefPtr<Gtk::EventControllerKey> m_key_controller;
};

template <typename T>
class GtkmmRenderer : public Renderer<T> {
public:
    GtkmmRenderer() : m_should_close(false), m_paused(false) {
        this->offset_x_ = T{0.0};
        this->offset_y_ = T{0.0};
        this->scale_ = T{150};
        this->rotation_x_ = T{0.0};
        this->rotation_y_ = T{0.0};
        this->rotation_z_ = T{0.0};
    }
    
    bool initialize() override {
        m_drawing_area = std::make_unique<NBodyDrawingArea<T>>();
        m_drawing_area->set_size_request(1024, 700);
        m_drawing_area->set_expand(true);
        
        m_graph_widget = std::make_unique<GraphWidget<T>>();

        m_control_box = std::make_unique<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);

        m_pause_button = std::make_unique<Gtk::Button>("Пауза");
        m_pause_button->signal_clicked().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_pause_clicked));
        m_control_box->append(*m_pause_button);

        m_reset_view_button = std::make_unique<Gtk::Button>("Сбросить вид");
        m_reset_view_button->signal_clicked().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_reset_view_clicked));
        m_control_box->append(*m_reset_view_button);
        m_control_box->set_margin(5);

        m_clear_trails_button = std::make_unique<Gtk::Button>("Очистить траектории");
        m_clear_trails_button->signal_clicked().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_clear_trails_clicked));
        m_control_box->append(*m_clear_trails_button);

        m_trail_length_label = std::make_unique<Gtk::Label>("Длина отслеживания траектории:");
        m_control_box->append(*m_trail_length_label);
        
        m_trail_length_scale = std::make_unique<Gtk::Scale>(Gtk::Orientation::HORIZONTAL);
        m_trail_length_scale->set_range(100, 1000);
        m_trail_length_scale->set_value(200);
        m_trail_length_scale->set_size_request(200, -1);
        m_trail_length_scale->signal_value_changed().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_trail_length_changed));
        m_control_box->append(*m_trail_length_scale);

        auto spacer = Gtk::make_managed<Gtk::Box>();
        spacer->set_hexpand(true);
        m_control_box->append(*spacer);
        
        m_render_button = std::make_unique<Gtk::Button>("Рендер");
        m_render_button->signal_clicked().connect(
            sigc::mem_fun(*this, &GtkmmRenderer::on_render_clicked));
        m_control_box->append(*m_render_button);
        
        return true;
    }
    
    void render(const System<T>& system) override {
        if (!m_drawing_area) {
            std::cerr << "ERROR: рендерер не инициализирован" << std::endl;
            return;
        }
        
        m_drawing_area->set_system(&system, this);
        m_drawing_area->add_trail_points();
        
        if (m_graph_widget) {
            m_graph_widget->add_point(system.graph_value());
        }
    }
    
    void update_display(const System<T>& system) {
        if (!m_drawing_area) {
            return;
        }
        
        m_drawing_area->set_system(&system, this);
        m_drawing_area->request_update();
    }
    
    bool process_events() override {
        if (!m_drawing_area) {
            return false;
        }

        return !m_should_close;
    }
    
    void shutdown() override {
        return;
    }
    
    // Методы для доступа к виджетам
    Gtk::Widget* get_drawing_area() { return m_drawing_area.get(); }
    Gtk::Widget* get_control_box() { return m_control_box.get(); }
    Gtk::Widget* get_graph_widget() { return m_graph_widget.get(); }
    
    void set_should_close(bool value) {
        m_should_close = value;
    }
    
    bool should_close() const {
        return m_should_close;
    }
    
    void on_reset_view_clicked() {
        if (!m_drawing_area) return;
    
        Vector<T> center_of_positions = m_drawing_area->get_center_of_positions();
        
        T min_x, max_x, min_y, max_y;
        m_drawing_area->get_bounds(min_x, max_x, min_y, max_y);
        
        int width = m_drawing_area->get_width();
        int height = m_drawing_area->get_height();
        
        T range_x = max_x - min_x;
        T range_y = max_y - min_y;
        
        if (range_x > T{0} && range_y > T{0}) {
            T scale_x = T(width * 0.9) / range_x;
            T scale_y = T(height * 0.9) / range_y;
            this->scale_ = (scale_x < scale_y) ? scale_x : scale_y;
        } else {
            this->scale_ = T{150};
        }
        
        this->offset_x_ = -center_of_positions.x() + T(width) / (T{2} * this->scale_);
        this->offset_y_ = -center_of_positions.y() + T(height) / (T{2} * this->scale_);
        this->rotation_x_ = T{0.0};
        this->rotation_y_ = T{0.0};
        this->rotation_z_ = T{0.0};
        
        m_drawing_area->queue_draw();
    }
    
    void on_clear_trails_clicked() {
        m_drawing_area->clear_trails();
    }
    
    void on_trail_length_changed() {
        int length = static_cast<int>(m_trail_length_scale->get_value());
        m_drawing_area->set_trail_length(length);
    }
    
    void on_pause_clicked() {
        m_paused = !m_paused;
        m_pause_button->set_label(m_paused ? "Продолжить" : "Пауза");
    }
    
    bool is_paused() const {
        return m_paused;
    }
    
    void set_paused(bool paused) {
        m_paused = paused;
        m_pause_button->set_label(m_paused ? "Продолжить" : "Пауза");
    }
    
    void on_render_clicked() {
        int width = m_drawing_area->get_width();
        int height = m_drawing_area->get_height();

        set_paused(true);
    
        auto toplevel = m_drawing_area->get_root();
        auto window = dynamic_cast<Gtk::Window*>(toplevel);
        
        if (!window) {
            std::cerr << "ERROR: Не удалось найти родительское окно" << std::endl;
            set_paused(false);
            return;
        }
        
        m_render_dialog = std::make_unique<RenderDialog>(*window);
        
        // Подключаем сигнал оценки времени
        m_render_dialog->signal_estimate_time.connect(
            sigc::mem_fun(*this, &GtkmmRenderer::estimate_render_time));
        
        m_render_dialog->signal_response().connect([this, width, height](int response_id) {
            if (response_id == Gtk::ResponseType::OK) {
                auto settings = m_render_dialog->get_settings();

                m_render_dialog->close();
                m_render_dialog.reset();

                Glib::signal_timeout().connect_once([this, settings, width, height]() {
                    signal_render_requested.emit(settings, width, height);
                }, 100);
            } else {
                m_render_dialog->close();
                m_render_dialog.reset();
                set_paused(false);
            }
        });

        m_render_dialog->set_modal(true);
        m_render_dialog->present();
    }

    void estimate_render_time(const RenderSettings& settings, int width, int height) {
        if (!m_drawing_area) return;
        
        auto system = m_drawing_area->get_system();
        auto renderer = m_drawing_area->get_renderer();
        
        if (!system || !renderer) {
            // Если нет данных системы, используем примерное время
            if (m_render_dialog) {
                m_render_dialog->update_eta_from_measurement(0.1);
            }
            return;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Рендерим все режимы которые будут сохраняться
        int render_count = 0;
        
        if (settings.save_main) {
            auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
            auto cr = Cairo::Context::create(surface);
            
            auto& cairo_renderer = m_drawing_area->get_cairo_renderer();
            auto temp_cairo = cairo_renderer;
            temp_cairo.set_depth_mode(false);
            temp_cairo.render(cr, *system, *renderer, width, height, false);
            render_count++;
        }
        
        if (settings.save_depth) {
            auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
            auto cr = Cairo::Context::create(surface);
            
            auto& cairo_renderer = m_drawing_area->get_cairo_renderer();
            auto temp_cairo = cairo_renderer;
            temp_cairo.set_depth_mode(true);
            temp_cairo.render(cr, *system, *renderer, width, height, false);
            render_count++;
        }
        
        if (settings.save_energy) {
            // Симулируем рендер графика энергии
            std::vector<T> dummy_history = {T{-1.0}, T{-1.1}, T{-0.9}};
            
            auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
            auto cr = Cairo::Context::create(surface);
            cr->set_source_rgb(0.0, 0.0, 0.0);
            cr->paint();
            
            // Простой рендер линии для симуляции графика
            cr->set_source_rgb(0.0, 1.0, 0.0);
            cr->set_line_width(2.0);
            cr->move_to(50, height/2);
            cr->line_to(width-50, height/2 + 10);
            cr->stroke();
            render_count++;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double total_time = duration.count() / 1000000.0;  // Конвертируем в секунды
        
        // Время на один кадр = общее время / количество рендеров
        double time_per_frame = render_count > 0 ? total_time / render_count : 0.1;
        
        // Добавляем время для энкодирования видео (примерно 15% от времени рендера)
        time_per_frame *= 1.15;
        
        // Минимальное время - 0.001 секунды на кадр
        time_per_frame = std::max(0.001, time_per_frame);
        
        // Обновляем диалог с измеренным временем
        if (m_render_dialog) {
            m_render_dialog->update_eta_from_measurement(time_per_frame);
        }
    }

    sigc::signal<void(const RenderSettings&, int, int)> signal_render_requested;
    
private:
    std::unique_ptr<NBodyDrawingArea<T>> m_drawing_area;
    std::unique_ptr<Gtk::Box> m_control_box;
    std::unique_ptr<GraphWidget<T>> m_graph_widget;
    
    // Элементы управления
    std::unique_ptr<Gtk::Button> m_reset_view_button;
    std::unique_ptr<Gtk::Button> m_clear_trails_button;
    std::unique_ptr<Gtk::Button> m_pause_button;
    std::unique_ptr<Gtk::Label> m_trail_length_label;
    std::unique_ptr<Gtk::Scale> m_trail_length_scale;
    std::unique_ptr<Gtk::Button> m_render_button;
    
    // Флаг, указывающий на необходимость завершения
    std::atomic<bool> m_should_close;
    bool m_paused = false;
    
    std::unique_ptr<RenderDialog> m_render_dialog;
};

} // namespace nbody 