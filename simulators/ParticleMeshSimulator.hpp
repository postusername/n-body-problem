#pragma once

#include "simulators/Simulator.hpp"
#include "core/Body.hpp"
#include "core/Vector.hpp"
#include <vector>
#include <complex>
#include <fftw3.h>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace nbody {

template <typename T>
class ParticleMeshSimulator : public Simulator<T> {
private:
    int grid_size_;
    T box_size_;
    T cell_size_;
    T g_;
    int total_cells_;
    int fft_size_;
    T softening_; 
    
    // Адаптивный контроль сетки
    T min_cell_size_;         // Минимальный размер ячейки
    T max_cell_size_;         // Максимальный размер ячейки
    bool adaptive_box_;       // Включить адаптивное изменение размера
    int out_of_bounds_count_; // Счетчик выходов за границы
    
    // Режим вычисления сил
    enum ForceMode { PRECOMPUTED, LAZY };
    ForceMode force_mode_;
    mutable std::vector<bool> force_computed_; // Кэш для ленивого режима
    
    // Сетки для PM метода
    std::vector<T> density_grid_;
    std::vector<T> potential_grid_;
    mutable std::vector<Vector<T>> force_grid_;
    
    // FFTW данные
    double* fft_in_;
    fftw_complex* fft_out_;
    fftw_plan plan_forward_;
    fftw_plan plan_backward_;
    
    // Границы симуляции
    Vector<T> box_min_;
    Vector<T> box_max_;
    bool auto_box_size_;

public:
    explicit ParticleMeshSimulator(int grid_size = 64, double box_size = 0.0) 
        : grid_size_(grid_size), box_size_(T(box_size)), g_(T(1)),
          min_cell_size_(T(0.001)), max_cell_size_(T(10.0)),
          adaptive_box_(true), out_of_bounds_count_(0),
          auto_box_size_(box_size == 0.0),
          force_mode_(LAZY) {
        
        total_cells_ = grid_size_ * grid_size_ * grid_size_;
        fft_size_ = grid_size_ * grid_size_ * (grid_size_/2 + 1);
        int padded_size = grid_size_ * grid_size_ * 2 * (grid_size_/2 + 1);
        
        density_grid_.resize(total_cells_, T(0));
        potential_grid_.resize(total_cells_, T(0));
        force_grid_.resize(total_cells_, Vector<T>(T(0), T(0), T(0)));
        force_computed_.resize(total_cells_, false);

        fft_in_ = fftw_alloc_real(padded_size);
        fft_out_ = fftw_alloc_complex(fft_size_);
        if (!fft_in_ || !fft_out_)
            throw std::runtime_error("Failed to allocate FFTW memory");
        
        plan_forward_ = fftw_plan_dft_r2c_3d(grid_size_, grid_size_, grid_size_, 
                                             fft_in_, fft_out_, FFTW_MEASURE);
        plan_backward_ = fftw_plan_dft_c2r_3d(grid_size_, grid_size_, grid_size_, 
                                              fft_out_, fft_in_, FFTW_MEASURE);
        if (!plan_forward_ || !plan_backward_) {
            fftw_free(fft_in_);
            fftw_free(fft_out_);
            throw std::runtime_error("Failed to create FFTW plans");
        }
    }
    
    ~ParticleMeshSimulator() {
        fftw_destroy_plan(plan_forward_);
        fftw_destroy_plan(plan_backward_);
        fftw_free(fft_in_);
        fftw_free(fft_out_);
    }

    void set_g(T g) override {
        g_ = g;
    }

    void set_box_size(T box_size) {
        box_size_ = box_size;
        auto_box_size_ = false;
        update_grid_parameters();
    }

    const std::vector<T>& get_density_grid() const { return density_grid_; }
    const std::vector<T>& get_potential_grid() const { return potential_grid_; }
    const std::vector<Vector<T>>& get_force_grid() const { return force_grid_; }
    int get_grid_size() const { return grid_size_; }
    T get_cell_size() const { return cell_size_; }
    T get_box_size() const { return box_size_; }
    T get_softening() const { return softening_; }
    Vector<T> get_box_min() const { return box_min_; }
    Vector<T> get_box_max() const { return box_max_; }

    std::vector<double> get_fft_in_data() const {
        std::vector<double> data(total_cells_);
        for (int i = 0; i < total_cells_; ++i) {
            data[i] = fft_in_[i];
        }
        return data;
    }
    
    std::vector<std::complex<double>> get_fft_out_data() const {
        std::vector<std::complex<double>> data(fft_size_);
        for (int i = 0; i < fft_size_; ++i) {
            data[i] = std::complex<double>(fft_out_[i][0], fft_out_[i][1]);
        }
        return data;
    }

    bool step() override {
        if (!this->system_) return false;
        
        auto& bodies = this->system_->bodies();
        if (bodies.empty()) return false;
 
        if (auto_box_size_) {
            determine_simulation_box(bodies);
            auto_box_size_ = false;
        }

        out_of_bounds_count_ = 0;

        mass_assignment(bodies);
        solve_poisson_equation();
        compute_forces();
        integrate_equations_of_motion(bodies);

        if (adaptive_box_ && out_of_bounds_count_ > static_cast<int>(bodies.size()) / 4) {
            std::cerr << "ParticleMeshSimulator -- WARNING:Adapting box size due to " << out_of_bounds_count_ << " out-of-bounds particles" << std::endl;
            determine_simulation_box(bodies);
        }
        
        return true;
    }

    int get_out_of_bounds_count() const { return out_of_bounds_count_; }
    T get_min_cell_size() const { return min_cell_size_; }
    T get_max_cell_size() const { return max_cell_size_; }
    void set_adaptive_box(bool enable) { adaptive_box_ = enable; }
    void set_cell_size_limits(T min_size, T max_size) {
        min_cell_size_ = min_size;
        max_cell_size_ = max_size;
    }

    void set_force_mode_precomputed() { 
        force_mode_ = PRECOMPUTED; 
    }

    void set_force_mode_lazy() { 
        force_mode_ = LAZY;
        std::fill(force_computed_.begin(), force_computed_.end(), false);
    }

    bool is_force_mode_lazy() const { return force_mode_ == LAZY; }

private:
    void determine_simulation_box(const std::vector<Body<T>>& bodies) {
        if (bodies.empty()) return;
        
        Vector<T> min_pos = bodies[0].position();
        Vector<T> max_pos = bodies[0].position();
        T total_mass = T(0);
        Vector<T> center_of_mass(T(0), T(0), T(0));
    
        for (const auto& body : bodies) {
            const Vector<T>& pos = body.position();
            min_pos = Vector<T>(std::min(min_pos.x(), pos.x()),
                               std::min(min_pos.y(), pos.y()),
                               std::min(min_pos.z(), pos.z()));
            max_pos = Vector<T>(std::max(max_pos.x(), pos.x()),
                               std::max(max_pos.y(), pos.y()),
                               std::max(max_pos.z(), pos.z()));
            
            center_of_mass += body.position() * body.mass();
            total_mass += body.mass();
        }
        center_of_mass /= total_mass;

        T max_distance = T(0);
        for (const auto& body : bodies) {
            T dist = (body.position() - center_of_mass).magnitude();
            max_distance = std::max(max_distance, dist);
        }
        
        Vector<T> range = max_pos - min_pos;
        T span = std::max({range.x(), range.y(), range.z()});

        T system_size = std::max(span, T(2) * max_distance);
        T padding_factor = T(2);
        
        box_size_ = system_size * padding_factor;

        T cell_size = box_size_ / T(grid_size_);
        if (cell_size < min_cell_size_) {
            box_size_ = min_cell_size_ * T(grid_size_);
            cell_size = min_cell_size_;
        } else if (cell_size > max_cell_size_) {
            box_size_ = max_cell_size_ * T(grid_size_);
            cell_size = max_cell_size_;
        }

        Vector<T> half_box = Vector<T>(box_size_, box_size_, box_size_) * T(0.5);
        box_min_ = center_of_mass - half_box;
        box_max_ = center_of_mass + half_box;
        
        update_grid_parameters();
    }
    
    void update_grid_parameters() {
        cell_size_ = box_size_ / T(grid_size_);
        softening_ = T(2.8) * cell_size_;
    }

    void mass_assignment(const std::vector<Body<T>>& bodies) {
        std::fill(density_grid_.begin(), density_grid_.end(), T(0));
        
        for (const auto& body : bodies)
            assign_particle_mass_cic(body);
    }
    
    void assign_particle_mass_cic(const Body<T>& body) {
        T cell_volume = cell_size_ * cell_size_ * cell_size_;

        Vector<T> pos = body.position();
        bool out_of_bounds = (pos.x() < box_min_.x() || pos.x() > box_max_.x() ||
                              pos.y() < box_min_.y() || pos.y() > box_max_.y() ||
                              pos.z() < box_min_.z() || pos.z() > box_max_.z());
        
        if (out_of_bounds) {
            out_of_bounds_count_++;
            if (out_of_bounds_count_ > 5)
                std::cerr << "Warning: Particles going out of bounds. Consider increasing box size." << std::endl;
        }
        
        Vector<T> grid_pos = (body.position() - box_min_) / cell_size_;
        
        int i = static_cast<int>(std::floor(grid_pos.x()));
        int j = static_cast<int>(std::floor(grid_pos.y()));
        int k = static_cast<int>(std::floor(grid_pos.z()));

        T fx = grid_pos.x() - std::floor(grid_pos.x());
        T fy = grid_pos.y() - std::floor(grid_pos.y());
        T fz = grid_pos.z() - std::floor(grid_pos.z());
        
        // CIC интерполяция
        for (int dk = 0; dk <= 1; ++dk) {
            for (int dj = 0; dj <= 1; ++dj) {
                for (int di = 0; di <= 1; ++di) {
                    int gi = (i + di) % grid_size_;
                    int gj = (j + dj) % grid_size_;
                    int gk = (k + dk) % grid_size_;

                    gi = (gi + grid_size_) % grid_size_;
                    gj = (gj + grid_size_) % grid_size_;
                    gk = (gk + grid_size_) % grid_size_;
                    
                    T weight = (di == 0 ? T(1) - fx : fx) *
                               (dj == 0 ? T(1) - fy : fy) *
                               (dk == 0 ? T(1) - fz : fz);
                    
                    int idx = gk * grid_size_ * grid_size_ + gj * grid_size_ + gi;
                    density_grid_[idx] += body.mass() * weight / cell_volume;
                }
            }
        }
    }

    void solve_poisson_equation() {
        for (int i = 0; i < total_cells_; ++i)
            fft_in_[i] = static_cast<double>(density_grid_[i]);

        fftw_execute(plan_forward_);
        apply_greens_function();
        fftw_execute(plan_backward_);

        T norm = T(1) / T(grid_size_ * grid_size_ * grid_size_);
        for (int i = 0; i < total_cells_; ++i) {
            potential_grid_[i] = T(fft_in_[i]) * norm;
        }
    }
    
    void apply_greens_function() {
        T kfac = T(2) * T(M_PI) / box_size_;
        
        for (int iz = 0; iz < grid_size_; ++iz) {
            for (int iy = 0; iy < grid_size_; ++iy) {
                for (int ix = 0; ix <= grid_size_/2; ++ix) {
                    int idx = iz * grid_size_ * (grid_size_/2 + 1) + 
                              iy * (grid_size_/2 + 1) +
                              ix;

                    T kx = T(ix) * kfac;
                    T ky = (iy <= grid_size_/2) ? T(iy) * kfac : T(iy - grid_size_) * kfac;
                    T kz = (iz <= grid_size_/2) ? T(iz) * kfac : T(iz - grid_size_) * kfac;
                    
                    T k2 = kx*kx + ky*ky + kz*kz;
        
                    if (k2 > T(0)) {
                        T green_factor = T(-4.) * T(M_PI) * g_ / k2;
                        fft_out_[idx][0] *= static_cast<double>(green_factor);
                        fft_out_[idx][1] *= static_cast<double>(green_factor);
                    } else {
                        fft_out_[idx][0] = 0.0;
                        fft_out_[idx][1] = 0.0;
                    }
                }
            }
        }
    }

    void compute_forces() {
        if (force_mode_ == PRECOMPUTED) {
            for (int k = 0; k < grid_size_; ++k) {
                for (int j = 0; j < grid_size_; ++j) {
                    for (int i = 0; i < grid_size_; ++i) {
                        int idx = k * grid_size_ * grid_size_ + j * grid_size_ + i;
                        force_grid_[idx] = compute_force_at_grid_point(i, j, k);
                    }
                }
            }
        } else {
            std::fill(force_computed_.begin(), force_computed_.end(), false);
        }
    }
    
    Vector<T> compute_force_at_grid_point(int i, int j, int k) const {
        int idx = k * grid_size_ * grid_size_ + j * grid_size_ + i;
        if (force_mode_ == LAZY && force_computed_[idx])
            return force_grid_[idx];

        int ip = (i + 1) % grid_size_;
        int im = (i - 1 + grid_size_) % grid_size_;
        int jp = (j + 1) % grid_size_;
        int jm = (j - 1 + grid_size_) % grid_size_;
        int kp = (k + 1) % grid_size_;
        int km = (k - 1 + grid_size_) % grid_size_;
        
        T phi_ip = potential_grid_[k * grid_size_ * grid_size_ + j * grid_size_ + ip];
        T phi_im = potential_grid_[k * grid_size_ * grid_size_ + j * grid_size_ + im];
        T phi_jp = potential_grid_[k * grid_size_ * grid_size_ + jp * grid_size_ + i];
        T phi_jm = potential_grid_[k * grid_size_ * grid_size_ + jm * grid_size_ + i];
        T phi_kp = potential_grid_[kp * grid_size_ * grid_size_ + j * grid_size_ + i];
        T phi_km = potential_grid_[km * grid_size_ * grid_size_ + j * grid_size_ + i];
        
        T inv_2h = T(1) / (T(2) * cell_size_);
        
        // F = -∇φ
        Vector<T> force(-(phi_ip - phi_im) * inv_2h,
                        -(phi_jp - phi_jm) * inv_2h,
                        -(phi_kp - phi_km) * inv_2h);
        
        if (force_mode_ == LAZY) {
            force_grid_[idx] = force;
            force_computed_[idx] = true;
        }
        
        return force;
    }

    void integrate_equations_of_motion(std::vector<Body<T>>& bodies) {
        for (auto& body : bodies) {
            Vector<T> force = interpolate_force_cic(body.position());
            Vector<T> acceleration = force / body.mass();

            body.set_velocity(body.velocity() + acceleration * this->dt_);
            body.set_position(body.position() + body.velocity() * this->dt_);
        }
    }
    
    Vector<T> interpolate_force_cic(const Vector<T>& position) {
        Vector<T> grid_pos = (position - box_min_) / cell_size_;

        int i = static_cast<int>(std::floor(grid_pos.x()));
        int j = static_cast<int>(std::floor(grid_pos.y()));
        int k = static_cast<int>(std::floor(grid_pos.z()));

        T fx = grid_pos.x() - std::floor(grid_pos.x());
        T fy = grid_pos.y() - std::floor(grid_pos.y());
        T fz = grid_pos.z() - std::floor(grid_pos.z());
        
        Vector<T> force(T(0), T(0), T(0));
        
        // CIC интерполяция
        for (int dk = 0; dk <= 1; ++dk) {
            for (int dj = 0; dj <= 1; ++dj) {
                for (int di = 0; di <= 1; ++di) {
                    int gi = (i + di) % grid_size_;
                    int gj = (j + dj) % grid_size_;
                    int gk = (k + dk) % grid_size_;
                    
                    gi = (gi + grid_size_) % grid_size_;
                    gj = (gj + grid_size_) % grid_size_;
                    gk = (gk + grid_size_) % grid_size_;
                    
                    T weight = (di == 0 ? T(1) - fx : fx) *
                               (dj == 0 ? T(1) - fy : fy) *
                               (dk == 0 ? T(1) - fz : fz);
                    
                    Vector<T> grid_force;
                    if (force_mode_ == PRECOMPUTED) {
                        int idx = gk * grid_size_ * grid_size_ + gj * grid_size_ + gi;
                        grid_force = force_grid_[idx];
                    } else {
                        grid_force = compute_force_at_grid_point(gi, gj, gk);
                    }
                    
                    force += grid_force * weight;
                }
            }
        }
        
        return force;
    }
};

} // namespace nbody


