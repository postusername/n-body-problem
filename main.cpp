#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "core/Body.hpp"
#include "core/DoubleDouble.h"
#include "core/Vector.hpp"
#include "renderers/GtkmmRenderer.hpp"
#include "renderers/RenderEngine.hpp"
#include "simulators/NewtonianSimulator.hpp"
#include "systems/CircleSystem.hpp"
#include "systems/SolarSystem.hpp"
#include "systems/ThreeBodySystem.hpp"
#include "systems/TwoBodySystem.hpp"

#include <gtkmm.h>
#include <gio/gioenums.h>



class SimulationApp {
public:
    SimulationApp() : running(true), cli_dt_value(1e-5) {}
    
    int run(int argc, char* argv[]) {
        try {
            std::cout << "INFO: Инициализация GTK..." << std::endl;
            
            app = Gtk::Application::create("org.nbody.simulation");
            set_cli_options();
            
            app->signal_handle_local_options().connect(
                sigc::mem_fun(*this, &SimulationApp::on_handle_local_options), false);
            app->signal_activate().connect(sigc::mem_fun(*this, &SimulationApp::on_activate));
            app->signal_shutdown().connect(sigc::mem_fun(*this, &SimulationApp::on_shutdown));
            
            std::cout << "INFO: Создание системы..." << std::endl;
            system.generate();
            

            if (!system.is_valid()) {
                std::cerr << "ERROR: Начальное состояние системы некорректно" << std::endl;
                return 1;
            }
            
            simulator.set_system(&system);
            
            if (!renderer.initialize()) {
                std::cerr << "ERROR: Не удалось инициализировать рендерер" << std::endl;
                return 1;
            }
            
            window = std::make_unique<Gtk::Window>();
            window->set_title("N-Body Simulation");
            window->set_default_size(1280, 720);
            window->signal_close_request().connect(sigc::mem_fun(*this, &SimulationApp::on_window_close), false);
            
            notebook = std::make_unique<Gtk::Notebook>();
            
            main_box = std::make_unique<Gtk::Box>(Gtk::Orientation::VERTICAL, 5);
            main_box->append(*renderer.get_drawing_area());
            main_box->append(*renderer.get_control_box());
            
            graph_box = std::make_unique<Gtk::Box>(Gtk::Orientation::VERTICAL, 5);
            graph_box->append(*renderer.get_graph_widget());
            
            auto graph_controls = std::make_unique<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
            auto reset_graph_button = std::make_unique<Gtk::Button>("Сбросить вид графика");
            reset_graph_button->signal_clicked().connect([this]() {
                static_cast<nbody::GraphWidget<DoubleDouble>*>(renderer.get_graph_widget())->reset_view();
            });
            graph_controls->append(*reset_graph_button);
            graph_controls->set_margin(5);
            graph_box->append(*graph_controls);
            
            notebook->append_page(*main_box, "Симуляция");
            notebook->append_page(*graph_box, "График");
            
            window->set_child(*notebook);
            
            std::cout << "INFO: Запуск главного цикла GTK..." << std::endl;
            
            return app->run(argc, argv);
            
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
    void set_cli_options() {
        Glib::OptionEntry dt_entry;
        dt_entry.set_long_name("dt");
        dt_entry.set_description("Timestep for simulation (default = 1e-5)");
        dt_entry.set_arg_description("TIMESTEP");
        
        Glib::OptionGroup* group = new Glib::OptionGroup("nbody", "N-Body Simulation Options");
        group->add_entry(dt_entry, cli_dt_value);
        
        app->add_option_group(*group);
    }

    int on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>&) {
        return -1;
    }
    
    void on_activate() {
        if (!window) {
            std::cerr << "ERROR: Окно не было создано перед активацией приложения" << std::endl;
            app->quit();
            return;
        }
        
        simulator.set_dt(cli_dt_value);
        simulator.set_g(1);

        window->set_application(app);
        window->present();

        renderer.signal_render_requested.connect([this](const nbody::RenderSettings& settings, int width, int height) {
            this->start_render(settings, width, height);
        });
    
        Glib::signal_timeout().connect_once([this]() {
            std::cout << "INFO: Запуск симуляции в отдельном потоке..." << std::endl;
            simulation_thread = std::thread(&SimulationApp::simulation_loop, this);
        }, 100);
    }
    
    void on_shutdown() {
        std::cout << "INFO: Приложение завершается, останавливаем симуляцию..." << std::endl;
        running = false;
        
        if (simulation_thread.joinable()) {
            std::cout << "INFO: Ожидаем завершения потока симуляции..." << std::endl;
            simulation_thread.join();
        }
        
        std::cout << "INFO: Поток симуляции завершен" << std::endl;
    }
    
    void simulation_loop() {
        const int steps_per_frame = simulator.steps_per_frame();
        
        while (running) {
            auto frame_start = std::chrono::steady_clock::now();
            
            if (!renderer.is_paused()) {
                for (int i = 0; i < steps_per_frame && running; ++i) {
                    if (!simulator.step()) {
                        std::cerr << "ERROR: Ошибка в шаге симуляции" << std::endl;
                        running = false;
                        break;
                    }
                }
                renderer.render(system);
            } else {
                renderer.update_display(system);
            }
            
            if (!system.is_valid()) {
                std::cerr << "ERROR: Система стала некорректной, останавливаем симуляцию" << std::endl;
                running = false;

                auto dispatcher = Glib::Dispatcher();
                dispatcher.connect([this]() {
                    app->quit();
                });
                dispatcher.emit();
                break;
            }
            bool events_ok = renderer.process_events();
            
            if (!events_ok) {
                std::cout << "INFO: Окно закрыто, останавливаем симуляцию" << std::endl;
                running = false;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(17) - (std::chrono::steady_clock::now() - frame_start));
        }
    }
    
    void cleanup() {
        running = false;
        
        renderer.shutdown();
        
        window.reset();
        main_box.reset();
        graph_box.reset();
        notebook.reset();
        
        std::cout << "INFO: Программа завершена успешно" << std::endl;
    }
    
    void start_render(const nbody::RenderSettings& settings, int width, int height) {
        std::cout << "INFO: Запуск режима рендера..." << std::endl;

        running = false;
        
        if (simulation_thread.joinable()) {
            simulation_thread.join();
        }

        if (window) {
            window->close();
            window.reset();
        }

        nbody::RenderEngine<DoubleDouble> render_engine(settings, width, height);
        bool success = render_engine.execute(system, simulator, renderer);
        
        if (success) {
            std::cout << "INFO: Рендер завершен успешно" << std::endl;
        } else {
            std::cerr << "ERROR: Ошибка при выполнении рендера" << std::endl;
        }

        app->quit();
    }
    
    bool on_window_close() {
        std::cout << "INFO: Запрос на закрытие окна, завершаем приложение..." << std::endl;
        running = false;

        renderer.set_should_close(true);
        app->quit();
        
        return true;
    }
    
private:
    std::atomic<bool> running;
    Glib::RefPtr<Gtk::Application> app;
    std::unique_ptr<Gtk::Window> window;
    std::unique_ptr<Gtk::Notebook> notebook;
    std::unique_ptr<Gtk::Box> main_box;
    std::unique_ptr<Gtk::Box> graph_box;
    std::thread simulation_thread;
    double cli_dt_value;
    
    nbody::ThreeBodySystem<DoubleDouble> system;
    nbody::NewtonianSimulator<DoubleDouble> simulator;
    nbody::GtkmmRenderer<DoubleDouble> renderer;
};

int main(int argc, char* argv[]) {
    SimulationApp app;
    return app.run(argc, argv);
} 