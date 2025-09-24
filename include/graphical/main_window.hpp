#ifndef MICRO_COMPOSER_GUI_MAIN_WINDOW_HPP
#define MICRO_COMPOSER_GUI_MAIN_WINDOW_HPP

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <memory>

#include "graphical/step_grid_widget.hpp"
#include "graphical/sequencer_controller.hpp"

namespace Micro_composer {
namespace gui {

class MainWindow : public Gtk::Window {
public:
    explicit MainWindow(std::shared_ptr<SequencerController> controller);
    virtual ~MainWindow() = default;

protected:
    // Signal handlers
    void on_play_clicked();
    void on_stop_clicked();
    void on_clear_clicked();
    void update_status();

    // Keyboard event handlers
    bool on_key_press_event(GdkEventKey* key_event) override;

    // Child widgets
    Gtk::Box m_main_box;
    Gtk::Box m_control_box;
    Gtk::Button m_play_button;
    Gtk::Button m_stop_button;
    Gtk::Button m_clear_button;
    Gtk::Label m_status_label;

    std::unique_ptr<StepGridWidget> m_step_grid;
    std::shared_ptr<SequencerController> m_controller;

    // Update timer
    sigc::connection m_timer_connection;
    bool on_timeout();
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_MAIN_WINDOW_HPP