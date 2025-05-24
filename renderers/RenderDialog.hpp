#pragma once

#include <chrono>
#include <filesystem>

#include <gtkmm.h>



namespace nbody {

struct RenderSettings {
    std::string output_path = "./render_output";
    double duration = 10.0;
    double dt = 1e-2;
    bool save_main = true;
    bool save_depth = true;
    bool save_energy = true;
};

class RenderDialog : public Gtk::Dialog {
public:
    RenderDialog(Gtk::Window& parent);
    
    RenderSettings get_settings() const { 
        RenderSettings settings;
        settings.output_path = path_entry_->get_text();
        settings.duration = duration_spin_->get_value();
        settings.dt = dt_spin_->get_value();
        settings.save_main = main_check_->get_active();
        settings.save_depth = depth_check_->get_active();
        settings.save_energy = energy_check_->get_active();
        return settings;
    }
    
    // Сигнал для запроса оценки времени
    sigc::signal<void(const RenderSettings&, int, int)> signal_estimate_time;
    
    // Метод для обновления ETA на основе измеренного времени
    void update_eta_from_measurement(double time_per_frame);
    
private:
    void on_duration_changed();
    void on_dt_changed();
    void on_browse_clicked();
    void on_estimate_clicked();
    void on_save_options_changed();
    void update_frame_info();
    int count_active_modes() const;
    
    RenderSettings settings_;
    double measured_time_per_frame_ = -1.0; // -1 означает что замер не сделан
    int measured_modes_count_ = 0;          // Количество режимов для которых делалось измерение
    
    // Виджеты
    std::unique_ptr<Gtk::Entry> path_entry_;
    std::unique_ptr<Gtk::Button> browse_button_;
    std::unique_ptr<Gtk::SpinButton> duration_spin_;
    std::unique_ptr<Gtk::SpinButton> dt_spin_;
    std::unique_ptr<Gtk::CheckButton> main_check_;
    std::unique_ptr<Gtk::CheckButton> depth_check_;
    std::unique_ptr<Gtk::CheckButton> energy_check_;
    std::unique_ptr<Gtk::Label> frame_info_label_;
    std::unique_ptr<Gtk::Button> estimate_button_;
};

RenderDialog::RenderDialog(Gtk::Window& parent) 
    : Gtk::Dialog() {
    
    set_title("Настройки рендера");
    set_transient_for(parent);
    set_modal(true);
    set_default_size(400, 300);
    
    auto content = get_content_area();
    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_margin(20);
    
    int row = 0;
    
    // Путь для сохранения
    auto path_label = Gtk::make_managed<Gtk::Label>("Путь для сохранения:");
    path_label->set_halign(Gtk::Align::START);
    grid->attach(*path_label, 0, row, 1, 1);
    
    auto path_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 5);
    path_entry_ = std::make_unique<Gtk::Entry>();
    path_entry_->set_text(settings_.output_path);
    path_entry_->set_hexpand(true);
    path_box->append(*path_entry_);
    
    browse_button_ = std::make_unique<Gtk::Button>("Обзор...");
    browse_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &RenderDialog::on_browse_clicked));
    path_box->append(*browse_button_);
    
    grid->attach(*path_box, 1, row++, 1, 1);
    
    // Длительность записи
    auto duration_label = Gtk::make_managed<Gtk::Label>("Длительность (сек):");
    duration_label->set_halign(Gtk::Align::START);
    grid->attach(*duration_label, 0, row, 1, 1);
    
    duration_spin_ = std::make_unique<Gtk::SpinButton>();
    duration_spin_->set_range(0.1, 3600.0);
    duration_spin_->set_increments(0.1, 1.0);
    duration_spin_->set_digits(1);
    duration_spin_->set_value(settings_.duration);
    duration_spin_->signal_value_changed().connect(
        sigc::mem_fun(*this, &RenderDialog::on_duration_changed));
    grid->attach(*duration_spin_, 1, row++, 1, 1);
    
    // Временной шаг
    auto dt_label = Gtk::make_managed<Gtk::Label>("Временной шаг:");
    dt_label->set_halign(Gtk::Align::START);
    grid->attach(*dt_label, 0, row, 1, 1);
    
    dt_spin_ = std::make_unique<Gtk::SpinButton>();
    dt_spin_->set_range(1e-6, 1e-2);
    dt_spin_->set_increments(1e-6, 1e-5);
    dt_spin_->set_digits(6);
    dt_spin_->set_value(settings_.dt);
    dt_spin_->signal_value_changed().connect(
        sigc::mem_fun(*this, &RenderDialog::on_dt_changed));
    grid->attach(*dt_spin_, 1, row++, 1, 1);
    
    // Опции сохранения
    auto options_label = Gtk::make_managed<Gtk::Label>("Что сохранить:");
    options_label->set_halign(Gtk::Align::START);
    grid->attach(*options_label, 0, row, 2, 1);
    row++;
    
    main_check_ = std::make_unique<Gtk::CheckButton>("Основной режим");
    main_check_->set_active(settings_.save_main);
    main_check_->signal_toggled().connect(
        sigc::mem_fun(*this, &RenderDialog::on_save_options_changed));
    grid->attach(*main_check_, 0, row++, 2, 1);
    
    depth_check_ = std::make_unique<Gtk::CheckButton>("Режим глубины");
    depth_check_->set_active(settings_.save_depth);
    depth_check_->signal_toggled().connect(
        sigc::mem_fun(*this, &RenderDialog::on_save_options_changed));
    grid->attach(*depth_check_, 0, row++, 2, 1);
    
    energy_check_ = std::make_unique<Gtk::CheckButton>("График энергии системы");
    energy_check_->set_active(settings_.save_energy);
    energy_check_->signal_toggled().connect(
        sigc::mem_fun(*this, &RenderDialog::on_save_options_changed));
    grid->attach(*energy_check_, 0, row++, 2, 1);
    
    // Информация о кадрах
    frame_info_label_ = std::make_unique<Gtk::Label>();
    frame_info_label_->set_halign(Gtk::Align::START);
    frame_info_label_->set_markup("<i>Количество кадров: ... ETA: --:--</i>");
    grid->attach(*frame_info_label_, 0, row++, 2, 1);
    
    estimate_button_ = std::make_unique<Gtk::Button>("Оценка времени");
    estimate_button_->signal_clicked().connect(
        sigc::mem_fun(*this, &RenderDialog::on_estimate_clicked));
    grid->attach(*estimate_button_, 0, row++, 2, 1);
    
    content->append(*grid);
    
    // Кнопки
    add_button("Отмена", Gtk::ResponseType::CANCEL);
    add_button("Начать запись", Gtk::ResponseType::OK);
    
    update_frame_info();
}

void RenderDialog::on_duration_changed() {
    settings_.duration = duration_spin_->get_value();
    update_frame_info();
}

void RenderDialog::on_dt_changed() {
    settings_.dt = dt_spin_->get_value();

    measured_time_per_frame_ = -1.0;
    measured_modes_count_ = 0;
    estimate_button_->set_label("Оценка времени");
    
    update_frame_info();
}

void RenderDialog::on_browse_clicked() {
    auto dialog = Gtk::FileChooserNative::create("Выберите папку для сохранения",
                                                  *this,
                                                  Gtk::FileChooser::Action::SELECT_FOLDER,
                                                  "Выбрать",
                                                  "Отмена");
    
    if (!settings_.output_path.empty() && std::filesystem::exists(settings_.output_path)) {
        try {
            dialog->set_current_folder(Gio::File::create_for_path(settings_.output_path));
        } catch (...) {

        }
    }
    
    dialog->signal_response().connect([this, dialog](int response) {
        if (response == Gtk::ResponseType::OK) {
            auto file = dialog->get_file();
            if (file) {
                settings_.output_path = file->get_path();
                path_entry_->set_text(settings_.output_path);
            }
        }
    });
    
    dialog->show();
}

void RenderDialog::on_estimate_clicked() {
    auto parent_window = dynamic_cast<Gtk::Window*>(get_transient_for());
    if (!parent_window) return;
    
    int width = 1278;
    int height = 700;
    
    auto settings = get_settings();
    estimate_button_->set_sensitive(false);
    estimate_button_->set_label("Оценка...");

    signal_estimate_time.emit(settings, width, height);
}

void RenderDialog::on_save_options_changed() {
    settings_.save_main = main_check_->get_active();
    settings_.save_depth = depth_check_->get_active();
    settings_.save_energy = energy_check_->get_active();
    update_frame_info();
}

void RenderDialog::update_frame_info() {
    int frame_count = std::max(1, static_cast<int>(settings_.duration * 60));
    int current_modes = count_active_modes();
    
    double time_per_frame;
    std::string eta_note;
    
    if (measured_time_per_frame_ > 0 && measured_modes_count_ > 0) {
        double base_time_per_mode = measured_time_per_frame_ / measured_modes_count_;
        time_per_frame = base_time_per_mode * current_modes;
        eta_note = " (измерено)";
    } else {
        time_per_frame = current_modes * 0.05;
        eta_note = " (оценка)";
    }

    if (current_modes == 0) {
        time_per_frame = 0.01;
    }
    
    double total_time = frame_count * time_per_frame;
    
    int hours = static_cast<int>(total_time / 3600);
    int minutes = static_cast<int>((total_time - hours * 3600) / 60);
    int seconds = static_cast<int>(total_time - hours * 3600 - minutes * 60);
    
    std::string eta_str;
    if (hours > 0) {
        eta_str = std::to_string(hours) + ":" + 
                 (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                 (seconds < 10 ? "0" : "") + std::to_string(seconds);
    } else {
        eta_str = (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                 (seconds < 10 ? "0" : "") + std::to_string(seconds);
    }
    
    std::string info_text = "Кадров: " + std::to_string(frame_count) + 
                           ", ETA: " + eta_str + eta_note;
    frame_info_label_->set_markup("<i>" + info_text + "</i>");
}

void RenderDialog::update_eta_from_measurement(double time_per_frame) {
    measured_time_per_frame_ = time_per_frame;
    measured_modes_count_ = count_active_modes();

    estimate_button_->set_sensitive(true);
    estimate_button_->set_label("Переоценить");

    update_frame_info();
}

int RenderDialog::count_active_modes() const {
    int count = 0;
    if (main_check_->get_active()) count++;
    if (depth_check_->get_active()) count++;
    if (energy_check_->get_active()) count++;
    return count;
}

} // namespace nbody 