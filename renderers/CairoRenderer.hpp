#pragma once

#include <deque>
#include <iomanip>
#include <sstream>
#include <tuple>
#include <vector>

#include "core/Body.hpp"
#include "renderers/Renderer.hpp"
#include "systems/System.hpp"

#include <cairomm/cairomm.h>



namespace nbody {

template<typename T>
class CairoRenderer {
public:
    CairoRenderer();

    void set_trail_length(size_t length) { trail_length_ = length; }
    void set_depth_mode(bool depth_mode) { depth_mode_ = depth_mode; }
    bool get_depth_mode() const { return depth_mode_; }
    void clear_trails();

    void add_trail_points(const std::vector<Body<T>>& bodies);

    void render(const Cairo::RefPtr<Cairo::Context>& cr, 
                const System<T>& system, 
                const Renderer<T>& renderer,
                int width, int height,
                bool show_info_text = false,
                double mouse_x = 0, double mouse_y = 0,
                bool mouse_in_area = false);
    
private:
    T get_depth(T x, T y, T z, const Renderer<T>& renderer) const;
    void get_depth_color(T depth, T min_depth, T max_depth, double& r, double& g, double& b) const;
    void render_trails(const Cairo::RefPtr<Cairo::Context>& cr, 
                       const Renderer<T>& renderer,
                       T min_depth, T max_depth);
    void render_bodies(const Cairo::RefPtr<Cairo::Context>& cr,
                       const std::vector<Body<T>>& bodies,
                       const Renderer<T>& renderer,
                       T min_depth, T max_depth);
    void render_info_text(const Cairo::RefPtr<Cairo::Context>& cr,
                          const Renderer<T>& renderer,
                          int width, int height,
                          double mouse_x, double mouse_y);

    std::vector<std::deque<std::tuple<T, T, T>>> trails_;
    size_t trail_length_ = 200;
    bool depth_mode_ = false;
    
    std::vector<std::tuple<double, double, double>> base_colors_ = {
        {0.2, 0.4, 1.0},
        {1.0, 0.5, 0.1},
        {1.0, 1.0, 1.0}
    };
};

template<typename T>
CairoRenderer<T>::CairoRenderer() {
}

template<typename T>
void CairoRenderer<T>::clear_trails() {
    trails_.clear();
}

template<typename T>
void CairoRenderer<T>::add_trail_points(const std::vector<Body<T>>& bodies) {
    if (trails_.size() != bodies.size()) {
        trails_.resize(bodies.size());
    }
    
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& body = bodies[i];
        auto& trail = trails_[i];

        trail.push_back(std::make_tuple(body.position().x(), body.position().y(), body.position().z()));

        while (trail.size() > trail_length_) {
            trail.pop_front();
        }
    }
}

template<typename T>
T CairoRenderer<T>::get_depth(T x, T y, T z, const Renderer<T>& renderer) const {
    T cx = cos(-renderer.rotation_x()), sx = sin(-renderer.rotation_x());
    T cy = cos(-renderer.rotation_y()), sy = sin(-renderer.rotation_y());
    
    T x1 = x;
    T z1 = y * sx + z * cx;
    T z2 = -x1 * sy + z1 * cy;
    
    return z2;
}

template<typename T>
void CairoRenderer<T>::get_depth_color(T depth, T min_depth, T max_depth, double& r, double& g, double& b) const {
    if (max_depth == min_depth) {
        r = g = b = 0.5;
        return;
    }
    
    T normalized = (depth - min_depth) / (max_depth - min_depth);
    double norm_val = static_cast<double>(normalized);
    
    double gray_level = 0.8 - norm_val * 0.7;
    r = g = b = gray_level;
}

template<typename T>
void CairoRenderer<T>::render(const Cairo::RefPtr<Cairo::Context>& cr, 
                              const System<T>& system, 
                              const Renderer<T>& renderer,
                              int width, int height,
                              bool show_info_text,
                              double mouse_x, double mouse_y,
                              bool mouse_in_area) {
    if (depth_mode_) {
        cr->set_source_rgb(0.9, 0.9, 0.9);
    } else {
        cr->set_source_rgb(0.0, 0.0, 0.0);
    }
    cr->paint();
    
    const auto& bodies = system.bodies();
    
    T min_depth = T{0}, max_depth = T{0};
    if (depth_mode_ && !bodies.empty()) {
        std::vector<T> visible_depths;

        for (const auto& body : bodies) {
            int screen_x = renderer.to_screen_x(body.position().x(), body.position().y(), body.position().z());
            int screen_y = renderer.to_screen_y(body.position().x(), body.position().y(), body.position().z());

            if (screen_x >= -50 && screen_x <= width + 50 && screen_y >= -50 && screen_y <= height + 50) {
                T depth = get_depth(body.position().x(), body.position().y(), body.position().z(), renderer);
                visible_depths.push_back(depth);
            }
        }

        // Вычисляем глубины для траекторий
        for (const auto& trail : trails_) {
            for (const auto& point : trail) {
                int screen_x = renderer.to_screen_x(std::get<0>(point), std::get<1>(point), std::get<2>(point));
                int screen_y = renderer.to_screen_y(std::get<0>(point), std::get<1>(point), std::get<2>(point));
                
                if (screen_x >= -50 && screen_x <= width + 50 && screen_y >= -50 && screen_y <= height + 50) {
                    T depth = get_depth(std::get<0>(point), std::get<1>(point), std::get<2>(point), renderer);
                    visible_depths.push_back(depth);
                }
            }
        }

        if (!visible_depths.empty()) {
            min_depth = max_depth = visible_depths[0];
            for (T depth : visible_depths) {
                if (depth < min_depth) min_depth = depth;
                if (depth > max_depth) max_depth = depth;
            }
        }
    }

    render_trails(cr, renderer, min_depth, max_depth);
    render_bodies(cr, bodies, renderer, min_depth, max_depth);
    if (show_info_text && mouse_in_area) {
        render_info_text(cr, renderer, width, height, mouse_x, mouse_y);
    }
}

template<typename T>
void CairoRenderer<T>::render_trails(const Cairo::RefPtr<Cairo::Context>& cr, 
                                     const Renderer<T>& renderer,
                                     T min_depth, T max_depth) {
    for (size_t i = 0; i < trails_.size(); ++i) {
        const auto& trail = trails_[i];
        if (trail.empty()) continue;
        
        if (depth_mode_) {
            // Рендеринг каждого сегмента с цветом, зависящим от глубины
            for (auto it = trail.begin(); it != trail.end(); ++it) {
                auto next_it = std::next(it);
                if (next_it == trail.end()) break;

                T depth = get_depth(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it), renderer);
                double r, g, b;
                get_depth_color(depth, min_depth, max_depth, r, g, b);

                size_t point_index = std::distance(trail.begin(), it);
                double age_factor = static_cast<double>(point_index) / static_cast<double>(trail.size());
                double alpha = 0.3 + 0.7 * age_factor;
                
                cr->set_source_rgba(r, g, b, alpha);
                cr->set_line_width(2.0);
                
                int x1 = renderer.to_screen_x(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
                int y1 = renderer.to_screen_y(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
                int x2 = renderer.to_screen_x(std::get<0>(*next_it), std::get<1>(*next_it), std::get<2>(*next_it));
                int y2 = renderer.to_screen_y(std::get<0>(*next_it), std::get<1>(*next_it), std::get<2>(*next_it));
                
                cr->move_to(x1, y1);
                cr->line_to(x2, y2);
                cr->stroke();
            }
        } else {
            // Обычный режим - цветные траектории
            if (i < base_colors_.size()) {
                auto color = base_colors_[i];
                cr->set_source_rgba(
                    std::get<0>(color), 
                    std::get<1>(color), 
                    std::get<2>(color), 
                    0.6
                );
            } else {
                cr->set_source_rgba(1.0, 1.0, 1.0, 0.6);
            }
            cr->set_line_width(1.5);
            
            bool first = true;
            for (const auto& point : trail) {
                int x = renderer.to_screen_x(std::get<0>(point), std::get<1>(point), std::get<2>(point));
                int y = renderer.to_screen_y(std::get<0>(point), std::get<1>(point), std::get<2>(point));
                
                if (first) {
                    cr->move_to(x, y);
                    first = false;
                } else {
                    cr->line_to(x, y);
                }
            }
            
            cr->stroke();
        }
    }
}

template<typename T>
void CairoRenderer<T>::render_bodies(const Cairo::RefPtr<Cairo::Context>& cr,
                                     const std::vector<Body<T>>& bodies,
                                     const Renderer<T>& renderer,
                                     T min_depth, T max_depth) {
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& body = bodies[i];
        int x = renderer.to_screen_x(body.position().x(), body.position().y(), body.position().z());
        int y = renderer.to_screen_y(body.position().x(), body.position().y(), body.position().z());
        
        if (depth_mode_) {
            T depth = get_depth(body.position().x(), body.position().y(), body.position().z(), renderer);
            double r, g, b;
            get_depth_color(depth, min_depth, max_depth, r, g, b);
            
            cr->set_source_rgb(r, g, b);
            cr->arc(x, y, 6.0, 0.0, 2.0 * M_PI);
            cr->fill();
            
            cr->set_source_rgb(1.0, 1.0, 1.0);
            cr->set_line_width(1.0);
            cr->arc(x, y, 6.0, 0.0, 2.0 * M_PI);
            cr->stroke();
        } else {
            if (i < base_colors_.size()) {
                auto color = base_colors_[i];
                
                Cairo::RefPtr<Cairo::RadialGradient> gradient = 
                    Cairo::RadialGradient::create(x, y, 0, x, y, 15.0);
                
                gradient->add_color_stop_rgba(0, 
                    std::get<0>(color), 
                    std::get<1>(color), 
                    std::get<2>(color), 
                    0.8);
                gradient->add_color_stop_rgba(1, 
                    std::get<0>(color), 
                    std::get<1>(color), 
                    std::get<2>(color), 
                    0.0);
                
                cr->set_source(gradient);
                cr->arc(x, y, 15.0, 0.0, 2.0 * M_PI);
                cr->fill();
                
                cr->set_source_rgb(1.0, 1.0, 1.0);
            } else {
                cr->set_source_rgb(1.0, 1.0, 1.0);
            }
            cr->arc(x, y, 4.0, 0.0, 2.0 * M_PI);
            cr->fill();
        }
    }
}

template<typename T>
void CairoRenderer<T>::render_info_text(const Cairo::RefPtr<Cairo::Context>& cr,
                                        const Renderer<T>& renderer,
                                        int width, int height,
                                        double mouse_x, double mouse_y) {
    T model_x = renderer.to_model_x(static_cast<int>(mouse_x), static_cast<int>(mouse_y));
    T model_y = renderer.to_model_y(static_cast<int>(mouse_x), static_cast<int>(mouse_y));

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "x: " << static_cast<double>(model_x) << ", y: " << static_cast<double>(model_y) << "   ";
    oss << "Rot X: " << static_cast<double>(renderer.rotation_x() * 180.0 / M_PI) << "°, "
        << "Rot Y: " << static_cast<double>(renderer.rotation_y() * 180.0 / M_PI) << "°";
    if (depth_mode_) {
        oss << " [Режим глубины]";
    }
    std::string coords_text = oss.str();

    cr->select_font_face("Sans", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_size(12);

    Cairo::TextExtents text_extents;
    cr->get_text_extents(coords_text, text_extents);
    
    double text_x = width - text_extents.width - 10;
    double text_y = height - 10;
    
    if (depth_mode_) {
        cr->set_source_rgba(1.0, 1.0, 1.0, 0.7);
    } else {
        cr->set_source_rgba(0.0, 0.0, 0.0, 0.7);
    }
    cr->rectangle(text_x - 5, text_y - text_extents.height - 2, 
                 text_extents.width + 10, text_extents.height + 6);
    cr->fill();
    
    if (depth_mode_) {
        cr->set_source_rgb(0.0, 0.0, 0.0);
    } else {
        cr->set_source_rgb(1.0, 1.0, 1.0);
    }
    cr->move_to(text_x, text_y);
    cr->show_text(coords_text);
}

} // namespace nbody 