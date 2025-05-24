#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "systems/System.hpp"



namespace nbody {

template <typename T>
class SolarSystem : public System<T> {
private:
    const T MAIN_BELT_AVG_MASS = T{4.78e13};
    const T KUIPER_BELT_AVG_MASS = T{5.0e16};
    const T MAIN_BELT_DENSITY = T{2.5e3};
    const T KUIPER_BELT_DENSITY = T{1.0e3};

public:
    SolarSystem() = default;
    
    void generate() override {
        this->clear();
        
        const T G = T{1.};
        const T M_SUN = T{1.989e30};
        const T AU = T{1.496e11};
        const T PI = T{3.14159265358979323846};

        this->add_body(Body<T>(M_SUN, Vector<T>(T{0}, T{0}, T{0}), Vector<T>(T{0}, T{0}, T{0}), "Sun"));
        
        add_planet("Mercury", T{3.30e23}, T{0.387}, T{0.2056}, T{7.00}, T{29.12}, T{48.33}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Venus", T{4.87e24}, T{0.723}, T{0.0068}, T{3.39}, T{54.88}, T{76.68}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Earth", T{5.97e24}, T{1.000}, T{0.0167}, T{0.00}, T{114.21}, T{348.74}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Mars", T{6.42e23}, T{1.524}, T{0.0934}, T{1.85}, T{49.56}, T{286.50}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Jupiter", T{1.90e27}, T{5.204}, T{0.0489}, T{1.30}, T{100.46}, T{275.07}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Saturn", T{5.68e26}, T{9.582}, T{0.0565}, T{2.49}, T{113.67}, T{339.39}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Uranus", T{8.68e25}, T{19.218}, T{0.0463}, T{0.77}, T{74.00}, T{96.54}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Neptune", T{1.02e26}, T{30.070}, T{0.0095}, T{1.77}, T{131.78}, T{276.34}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Ceres", T{9.39e20}, T{2.77}, T{0.0758}, T{10.59}, T{80.33}, T{73.12}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Vesta", T{2.59e20}, T{2.36}, T{0.0887}, T{7.14}, T{103.85}, T{150.73}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Pallas", T{2.11e20}, T{2.77}, T{0.2313}, T{34.84}, T{173.09}, T{310.05}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Hygiea", T{8.67e19}, T{3.14}, T{0.1126}, T{3.84}, T{283.20}, T{312.32}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Pluto", T{1.31e22}, T{39.482}, T{0.2488}, T{17.14}, T{110.30}, T{113.76}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Eris", T{1.66e22}, T{67.8}, T{0.4361}, T{44.04}, T{35.95}, T{150.98}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Haumea", T{4.01e21}, T{43.1}, T{0.1913}, T{28.19}, T{121.79}, T{239.08}, T{0.0}, G, M_SUN, AU, PI);
        add_planet("Makemake", T{3.1e21}, T{45.8}, T{0.1610}, T{29.01}, T{79.36}, T{297.24}, T{0.0}, G, M_SUN, AU, PI);

        std::vector<std::string> main_belt_files = {"main_belt_test.csv"};
        std::vector<std::string> kuiper_belt_files = {"kuiper_belt_test.csv"};
        std::cout << "SolarSystem -- INFO: Загрузка астероидов главного пояса..." << std::endl;
        load_main_belt(main_belt_files);
        std::cout << "SolarSystem -- INFO: Загрузка объектов пояса Койпера..." << std::endl;
        load_kuiper_belt(kuiper_belt_files);

        shift_to_barycenter();
    }
    
    T graph_value() const override {
        return compute_total_energy();
    }
    
    struct BeltLoadResult {
        int total_loaded = 0;
        int total_skipped = 0;
        T mass_sum = T{0};
    };

    struct FileLoadResult {
        int loaded = 0;
        int skipped = 0;
        T mass_sum = T{0};
    };

    BeltLoadResult load_belt(const std::vector<std::string>& filenames, bool is_kuiper) {
        BeltLoadResult result;
        const std::string belt_name = is_kuiper ? "пояса Койпера" : "главного пояса";

        for (const auto& filename : filenames) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "SolarSystem -- ERROR: не удалось открыть файл " << belt_name << " " << filename << "\n";
                continue;
            }

            FileLoadResult file_result = process_file(file, filename);
            result.total_loaded += file_result.loaded;
            result.total_skipped += file_result.skipped;
            result.mass_sum += file_result.mass_sum;
            
            std::cout << "SolarSystem -- INFO: " << belt_name << ", файл " << filename 
                      << ": загружено " << file_result.loaded << " тел, пропущено " << file_result.skipped << "\n";
        }

        std::cout << "SolarSystem -- INFO: " << belt_name << ", всего: загружено " 
                  << result.total_loaded << " тел, пропущено " << result.total_skipped << "\n";
        std::cout << "SolarSystem -- INFO: Суммарная масса " << belt_name << ": " 
                  << result.mass_sum << " кг\n";

        return result;
    }

    void load_main_belt(const std::vector<std::string>& filenames) {
        load_belt(filenames, false);
    }

    void load_kuiper_belt(const std::vector<std::string>& filenames) {
        load_belt(filenames, true);
    }

private:
    FileLoadResult process_file(std::ifstream& file, const std::string& filename) {
        FileLoadResult result;
        std::string line;
        std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
            try {
                auto [success, body_data] = parse_line(line, filename);
                if (!success) {
                    result.skipped++;
                    continue;
                }

                auto [name, mass, a, e, i, om, w, ma] = body_data;
                if (!validate_orbital_parameters(a, e, name, filename)) {
                    result.skipped++;
                    continue;
                }

                add_asteroid_body(name, mass, a, e, i, om, w, ma);
                result.loaded++;
                result.mass_sum += mass;
            } catch (const std::exception& ex) {
                std::cerr << "SolarSystem -- WARNING: Ошибка парсинга строки в " << filename 
                          << ": " << line << " (" << ex.what() << ")\n";
                result.skipped++;
            }
        }
        return result;
    }

    std::pair<bool, std::tuple<std::string, T, T, T, T, T, T, T>> parse_line(
        const std::string& line, const std::string& filename) {
        std::stringstream ss(line);
        std::vector<std::string> tokens;
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 8) {
            std::cerr << "SolarSystem -- WARNING: Пропущена строка в " << filename 
                      << " (недостаточно полей): " << line << "\n";
            return {false, {}};
        }

        std::string name = clean_name(tokens[0]);
        fix_decimal_points(tokens);

        T e = T(std::stod(tokens[2]));
        T a = T(std::stod(tokens[3]));
        T i = T(std::stod(tokens[4]));
        T om = T(std::stod(tokens[5]));
        T w = T(std::stod(tokens[7]));

        std::string gm_str = tokens[6];
        T gm = parse_gm(gm_str);
        T mass = estimate_mass(gm, T{0}, false);

        return {true, {name, mass, a, e, i, om, w, T{0}}};
    }

    std::string clean_name(const std::string& name) {
        std::string result = name;
        if (!result.empty() && result.front() == '"' && result.back() == '"') {
            result = result.substr(1, result.size() - 2);
        }
        result.erase(0, result.find_first_not_of(" \t"));
        result.erase(result.find_last_not_of(" \t") + 1);
        return result;
    }

    void fix_decimal_points(std::vector<std::string>& tokens) {
        for (int i : {2, 3, 4, 5, 7}) {
            if (tokens[i][0] == '.') {
                tokens[i] = "0" + tokens[i];
            }
        }
    }

    T parse_gm(const std::string& gm_str) {
        return (gm_str.empty() || gm_str == "null" || gm_str == "\"null\"") 
               ? T{0} : T(std::stod(gm_str));
    }

    bool validate_orbital_parameters(T a, T e, const std::string& name, const std::string& filename) {
        if (a <= T{0} || e < T{0} || e >= T{1}) {
            std::cerr << "SolarSystem -- WARNING: Пропущено тело " << name 
                      << " в " << filename << ": некорректные параметры\n";
            return false;
        }
        return true;
    }
    
private:
    T estimate_mass(T gm, T diameter, bool is_kuiper) {
        const T G = T{1.};
        const T PI = T{3.14159265358979323846};
        
        if (gm > T{0}) {
            return gm / G;
        }
        if (diameter > T{0}) {
            T radius = diameter * T{1e3} / T{2};
            T density = is_kuiper ? KUIPER_BELT_DENSITY : MAIN_BELT_DENSITY;
            return (T{4} / T{3}) * PI * radius * radius * radius * density;
        }
        return is_kuiper ? KUIPER_BELT_AVG_MASS : MAIN_BELT_AVG_MASS;
    }

    void add_asteroid_body(const std::string& name, T mass, T a, T e, T i_deg, T omega_deg, T Omega_deg, T ma_deg) {
        const T G = T{1.};
        const T M_SUN = T{1.989e30};
        const T AU = T{1.496e11};
        const T PI = T{3.14159265358979323846};
        
        T i = i_deg * PI / T{180};
        T omega = omega_deg * PI / T{180};
        T Omega = Omega_deg * PI / T{180};
        T ma = ma_deg * PI / T{180};
        T a_m = a * AU;
        
        T E = ma;
        for (int iter = 0; iter < 10000; ++iter) {
            T delta_E = (E - e * sin(E) - ma) / (T{1} - e * cos(E));
            E -= delta_E;
            if (abs(delta_E) < T{1e-10}) break;
        }
        
        T true_anomaly = T{2} * atan2(T(sqrt(T{1} + e) * sin(E / T{2})), 
                                      T(sqrt(T{1} - e) * cos(E / T{2})));
        T r = a_m * (T{1} - e * cos(E));
        
        T x_orb = r * cos(true_anomaly);
        T y_orb = r * sin(true_anomaly);
        
        T x = (cos(omega) * cos(Omega) - sin(omega) * sin(Omega) * cos(i)) * x_orb +
              (-sin(omega) * cos(Omega) - cos(omega) * sin(Omega) * cos(i)) * y_orb;
        T y = (cos(omega) * sin(Omega) + sin(omega) * cos(Omega) * cos(i)) * x_orb +
              (-sin(omega) * sin(Omega) + cos(omega) * cos(Omega) * cos(i)) * y_orb;
        T z = (sin(omega) * sin(i)) * x_orb + (cos(omega) * sin(i)) * y_orb;
        
        T mu = G * M_SUN;
        T p = a_m * (T{1} - e * e);
        T v_x_orb = -sqrt(mu / p) * sin(true_anomaly);
        T v_y_orb = sqrt(mu / p) * (e + cos(true_anomaly));
        
        T vx = (cos(omega) * cos(Omega) - sin(omega) * sin(Omega) * cos(i)) * v_x_orb +
               (-sin(omega) * cos(Omega) - cos(omega) * sin(Omega) * cos(i)) * v_y_orb;
        T vy = (cos(omega) * sin(Omega) + sin(omega) * cos(Omega) * cos(i)) * v_x_orb +
               (-sin(omega) * sin(Omega) + cos(omega) * cos(Omega) * cos(i)) * v_y_orb;
        T vz = (sin(omega) * sin(i)) * v_x_orb + (cos(omega) * sin(i)) * v_y_orb;
        
        this->add_body(Body<T>(mass, Vector<T>(x, y, z), Vector<T>(vx, vy, vz), name));
    }

    void add_planet(const std::string& name, T mass, T a_au, T e, T i_deg, T omega_deg, T Omega_deg, T ma_deg,
                   T G, T M_SUN, T AU, T PI) {
        // Преобразуем углы в радианы
        T i = i_deg * PI / T{180.0};
        T omega = omega_deg * PI / T{180.0};  
        T Omega = Omega_deg * PI / T{180.0};
        T ma = ma_deg * PI / T{180.0};
        T a = a_au * AU;
        
        // Решаем уравнение Кеплера для эксцентрической аномалии
        T E = ma;
        for (int iter = 0; iter < 10000; ++iter) {
            T delta_E = (E - e * sin(E) - ma) / (T{1.0} - e * cos(E));
            E -= delta_E;
            if (abs(delta_E) < T{1e-10}) break;
        }
        
        // Вычисляем истинную аномалию
        T true_anomaly = T{2.0} * atan2(T(sqrt(T{1.0} + e) * sin(E / T{2.0})), 
                                        T(sqrt(T{1.0} - e) * cos(E / T{2.0})));
        T r = a * (T{1.0} - e * cos(E));
        
        // Позиция в орбитальной плоскости
        T x_orb = r * cos(true_anomaly);
        T y_orb = r * sin(true_anomaly);
        
        // Поворот в экваториальную систему координат
        T x = (cos(omega) * cos(Omega) - sin(omega) * sin(Omega) * cos(i)) * x_orb +
              (-sin(omega) * cos(Omega) - cos(omega) * sin(Omega) * cos(i)) * y_orb;
        T y = (cos(omega) * sin(Omega) + sin(omega) * cos(Omega) * cos(i)) * x_orb +
              (-sin(omega) * sin(Omega) + cos(omega) * cos(Omega) * cos(i)) * y_orb;
        T z = (sin(omega) * sin(i)) * x_orb + (cos(omega) * sin(i)) * y_orb;
        
        // Скорости в орбитальной плоскости
        T mu = G * M_SUN;
        T p = a * (T{1.0} - e * e);
        T v_x_orb = -sqrt(mu / p) * sin(true_anomaly);
        T v_y_orb = sqrt(mu / p) * (e + cos(true_anomaly));
        
        // Поворот скоростей в экваториальную систему
        T vx = (cos(omega) * cos(Omega) - sin(omega) * sin(Omega) * cos(i)) * v_x_orb +
               (-sin(omega) * cos(Omega) - cos(omega) * sin(Omega) * cos(i)) * v_y_orb;
        T vy = (cos(omega) * sin(Omega) + sin(omega) * cos(Omega) * cos(i)) * v_x_orb +
               (-sin(omega) * sin(Omega) + cos(omega) * cos(Omega) * cos(i)) * v_y_orb;
        T vz = (sin(omega) * sin(i)) * v_x_orb + (cos(omega) * sin(i)) * v_y_orb;
        
        this->add_body(Body<T>(mass, Vector<T>(x, y, z), Vector<T>(vx, vy, vz), name));
    }
    
    void shift_to_barycenter() {
        T total_mass = T{0};
        Vector<T> barycenter_pos(T{0}, T{0}, T{0});
        Vector<T> barycenter_vel(T{0}, T{0}, T{0});
        
        // Вычисляем барицентр
        for (const auto& body : this->bodies()) {
            total_mass += body.mass();
            barycenter_pos = barycenter_pos + body.position() * body.mass();
            barycenter_vel = barycenter_vel + body.velocity() * body.mass();
        }
        
        if (total_mass > T{0}) {
            barycenter_pos = barycenter_pos * (T{1} / total_mass);
            barycenter_vel = barycenter_vel * (T{1} / total_mass);
        }
        
        // Сдвигаем все тела
        for (auto& body : this->bodies()) {
            body.set_position(body.position() - barycenter_pos);
            body.set_velocity(body.velocity() - barycenter_vel);
        }
    }
    
    T compute_total_energy() const {
        const T G = T{1};
        T kinetic = T{0};
        T potential = T{0};

        for (const auto& body : this->bodies()) {
            T v_squared = body.velocity().magnitude_squared();
            kinetic += T{0.5} * body.mass() * v_squared;
        }

        for (size_t i = 0; i < this->bodies().size(); ++i) {
            for (size_t j = i + 1; j < this->bodies().size(); ++j) {
                const auto& body_i = this->bodies()[i];
                const auto& body_j = this->bodies()[j];
                T distance = (body_i.position() - body_j.position()).magnitude();
                if (distance > T{0}) {
                    potential -= G * body_i.mass() * body_j.mass() / distance;
                }
            }
        }
        
        return kinetic + potential;
    }
};

} // namespace nbody 