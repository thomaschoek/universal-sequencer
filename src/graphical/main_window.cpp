#include "graphical/main_window.hpp"
#include <gtkmm/main.h>
#include <gtkmm/settings.h>
#include <gtkmm/cssprovider.h>
#include <gdkmm/screen.h>
#include <gdk/gdkkeysyms.h>
#include <glibmm/main.h>
#include <iostream>

namespace Micro_composer {
namespace gui {

MainWindow::MainWindow(std::shared_ptr<SequencerController> controller)
    : m_controller(controller),
      m_main_box(Gtk::ORIENTATION_VERTICAL, 10),
      m_control_box(Gtk::ORIENTATION_HORIZONTAL, 10),
      m_play_button("Play"),
      m_stop_button("Stop"),
      m_clear_button("Clear All"),
      m_status_label("Ready") {

    set_title("Micro Composer - Step Sequencer");
    set_default_size(600, 200);
    set_resizable(true);

    // Enable keyboard events
    set_can_focus(true);
    grab_focus();
    add_events(Gdk::KEY_PRESS_MASK);

    // Create step grid
    m_step_grid = std::make_unique<StepGridWidget>(m_controller);

    // Set up layout
    add(m_main_box);
    m_main_box.set_margin_top(15);
    m_main_box.set_margin_bottom(15);
    m_main_box.set_margin_left(15);
    m_main_box.set_margin_right(15);

    // Add step grid
    m_main_box.pack_start(*m_step_grid, Gtk::PACK_EXPAND_WIDGET, 10);

    // Add control buttons
    m_main_box.pack_start(m_control_box, Gtk::PACK_SHRINK, 5);
    m_control_box.set_homogeneous(false);
    m_control_box.pack_start(m_play_button, Gtk::PACK_SHRINK, 5);
    m_control_box.pack_start(m_stop_button, Gtk::PACK_SHRINK, 5);
    m_control_box.pack_start(m_clear_button, Gtk::PACK_SHRINK, 5);

    // Add status label
    m_main_box.pack_start(m_status_label, Gtk::PACK_SHRINK, 5);
    m_status_label.set_halign(Gtk::ALIGN_START);

    // Connect button signals
    m_play_button.signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::on_play_clicked));
    m_stop_button.signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::on_stop_clicked));
    m_clear_button.signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::on_clear_clicked));

    // Set up CSS for step styling
    auto css_provider = Gtk::CssProvider::create();
    auto screen = Gdk::Screen::get_default();
    auto style_context = Gtk::StyleContext::create();

    try {
        css_provider->load_from_data(
            "/* Step sequencer styles - compatible with system theme */\n"
            ".step-inactive {\n"
            "    background-color: rgba(45, 45, 45, 0.8);\n"
            "    color: rgba(136, 136, 136, 1.0);\n"
            "    border: 1px solid rgba(85, 85, 85, 1.0);\n"
            "    font-weight: normal;\n"
            "}\n"
            ".step-active {\n"
            "    background-color: rgba(74, 144, 226, 1.0);\n"
            "    color: rgba(255, 255, 255, 1.0);\n"
            "    border: 1px solid rgba(53, 122, 189, 1.0);\n"
            "    font-weight: normal;\n"
            "}\n"
            ".step-current {\n"
            "    background-color: rgba(245, 166, 35, 1.0);\n"
            "    color: rgba(255, 255, 255, 1.0);\n"
            "    border: 2px solid rgba(214, 137, 16, 1.0);\n"
            "    font-weight: bold;\n"
            "}\n"
            ".step-current-active {\n"
            "    background-color: rgba(231, 76, 60, 1.0);\n"
            "    color: rgba(255, 255, 255, 1.0);\n"
            "    border: 2px solid rgba(192, 57, 43, 1.0);\n"
            "    font-weight: bold;\n"
            "}\n"
        );

        style_context->add_provider_for_screen(screen, css_provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch (const Glib::Error& e) {
        std::cerr << "Error loading CSS: " << e.what() << std::endl;
    }

    // Set up timer for regular updates
    m_timer_connection = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &MainWindow::on_timeout), 50); // 20 FPS

    // Initial status update
    update_status();

    show_all_children();
}

void MainWindow::on_play_clicked() {
    try {
        if (m_controller) {
            m_controller->start();
            update_status();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error starting sequencer: " << e.what() << std::endl;
        m_status_label.set_text("Error: Failed to start sequencer");
    }
}

void MainWindow::on_stop_clicked() {
    try {
        if (m_controller) {
            m_controller->stop();
            update_status();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error stopping sequencer: " << e.what() << std::endl;
        m_status_label.set_text("Error: Failed to stop sequencer");
    }
}

void MainWindow::on_clear_clicked() {
    try {
        if (m_controller) {
            m_controller->clear_all_steps();
            m_step_grid->update_display();
            update_status();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error clearing steps: " << e.what() << std::endl;
        m_status_label.set_text("Error: Failed to clear steps");
    }
}

void MainWindow::update_status() {
    if (!m_controller) {
        m_status_label.set_text("Error: No controller");
        return;
    }

    if (!m_controller->is_sequencer_initialized()) {
        m_status_label.set_text("Error: Sequencer not initialized");
        return;
    }

    if (m_controller->is_running()) {
        m_status_label.set_text("Playing...");
        m_play_button.set_sensitive(false);
        m_stop_button.set_sensitive(true);
    } else {
        m_status_label.set_text("Ready");
        m_play_button.set_sensitive(true);
        m_stop_button.set_sensitive(false);
    }
}

bool MainWindow::on_timeout() {
    // Update the step grid display
    if (m_step_grid) {
        m_step_grid->update_display();
    }

    // Update status if needed
    update_status();

    return true; // Continue calling this function
}

bool MainWindow::on_key_press_event(GdkEventKey* key_event) {
    if (!m_controller) {
        return Gtk::Window::on_key_press_event(key_event);
    }

    // Handle number keys 1-8 for step toggling
    if (key_event->keyval >= GDK_KEY_1 && key_event->keyval <= GDK_KEY_8) {
        std::size_t step_index = key_event->keyval - GDK_KEY_1; // Convert to 0-based index

        if (step_index < m_controller->get_num_steps()) {
            try {
                bool current_state = m_controller->is_step_active(step_index);
                m_controller->set_step_active(step_index, !current_state);

                // Update the visual display
                if (m_step_grid) {
                    m_step_grid->update_display();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error toggling step " << (step_index + 1) << ": " << e.what() << std::endl;
            }
        }
        return true; // Event handled
    }

    // Handle spacebar for play/stop
    if (key_event->keyval == GDK_KEY_space) {
        try {
            if (m_controller->is_running()) {
                on_stop_clicked();
            } else {
                on_play_clicked();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling play/stop: " << e.what() << std::endl;
        }
        return true; // Event handled
    }

    // Let the base class handle other keys
    return Gtk::Window::on_key_press_event(key_event);
}

} // namespace gui
} // namespace Micro_composer