#include "core/Vector.hpp"
#include "core/Body.hpp"
#include "systems/ThreeBodySystem.hpp"
#include "systems/RingSystem.hpp"
#include "simulators/NewtonianSimulator.hpp"

// Добавляем прямое включение gtkmm
#include <gtkmm.h>

#include "renderers/GtkmmRenderer.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>

class SimulationApp {
public:
    SimulationApp() : running(true) {}
    
    int run(int argc, char* argv[]) {
        try {
            std::cout << "INFO: Инициализация GTK..." << std::endl;
            
            app = Gtk::Application::create(argc, argv, "org.nbody.simulation");
            app->signal_startup().connect(sigc::mem_fun(*this, &SimulationApp::on_app_startup));
            app->signal_shutdown().connect(sigc::mem_fun(*this, &SimulationApp::on_app_shutdown));
            
            std::cout << "INFO: Создание системы..." << std::endl;
            system.generate();
            
            if (!system.is_valid()) {
                std::cerr << "ERROR: Начальное состояние системы некорректно" << std::endl;
                return 1;
            }
            
            simulator.set_system(&system);
            simulator.set_dt(1e-5);
            simulator.set_g(2.5);
            
            if (!renderer.initialize()) {
                std::cerr << "ERROR: Не удалось инициализировать рендерер" << std::endl;
                return 1;
            }
            
            std::cout << "INFO: Запуск симуляции в отдельном потоке..." << std::endl;
            simulation_thread = std::thread(&SimulationApp::simulation_loop, this);
            
            std::cout << "INFO: Запуск главного цикла GTK..." << std::endl;
            return app->run();
            
        } catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "ERROR: Неизвестная ошибка!" << std::endl;
            return 1;
        }
    }
    
    ~SimulationApp() {
        cleanup();
    }
    
private:
    void on_app_startup() {
        window = new Gtk::Window();
        window->set_title("N-Body Simulation - Ring System");
        window->set_default_size(1280, 720);
        window->signal_delete_event().connect(
            sigc::mem_fun(*this, &SimulationApp::on_window_delete));
              
        Gtk::Box* main_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5));
        main_box->pack_start(*renderer.get_drawing_area(), true, true, 0);
        main_box->pack_start(*renderer.get_control_box(), false, false, 5);
        
        window->add(*main_box);
        window->show_all();
        
        app->add_window(*window);
    }

    void simulation_loop() {
        const int steps_per_frame = 1000;
        
        while (running) {
            for (int i = 0; i < steps_per_frame && running; ++i) {
                if (!simulator.step()) {
                    std::cerr << "ERROR: Ошибка в шаге симуляции" << std::endl;
                    running = false;
                    break;
                }
            }
            
            renderer.render(system);
            
            if (!system.is_valid()) {
                std::cerr << "ERROR: Система стала некорректной, останавливаем симуляцию" << std::endl;
                running = false;
                
                app->quit();
                break;
            }
            
            if (!renderer.process_events()) {
                std::cout << "INFO: Окно закрыто, останавливаем симуляцию" << std::endl;
                running = false;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        std::cout << "INFO: Поток симуляции завершен" << std::endl;
    }
    
    void cleanup() {
        running = false;
        
        if (simulation_thread.joinable()) {
            simulation_thread.join();
        }
        renderer.shutdown();
        
        delete window;
        window = nullptr;
        
        std::cout << "INFO: Программа завершена успешно" << std::endl;
    }
    
    bool on_window_delete(GdkEventAny* event) {
        running = false;
        app->quit();
        return false;
    }
    
    void on_app_shutdown() {
        cleanup();
    }
    
private:
    std::atomic<bool> running;
    Glib::RefPtr<Gtk::Application> app;
    Gtk::Window* window = nullptr;
    std::thread simulation_thread;
    
    nbody::RingSystem<long double> system;
    nbody::NewtonianSimulator<long double> simulator;
    nbody::GtkmmRenderer<long double> renderer;
};

int main(int argc, char* argv[]) {
    SimulationApp app;
    return app.run(argc, argv);
} 