#include "graphical/step_parameter_menu.hpp"
#include <gtkmm/cssprovider.h>
#include <gdkmm/screen.h>
#include <gdk/gdkkeysyms.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace Micro_composer {
namespace gui {

StepParameterMenu::StepParameterMenu(std::shared_ptr<SequencerController> controller,
                                     std::size_t step_index)
    : m_controller(controller),
      m_step_index(step_index),
      m_selected_parameter(0),
      m_main_box(Gtk::ORIENTATION_VERTICAL, 0),
      m_numeric_input(""),
      m_has_numeric_input(false) {

    // Set up window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_POPUP_MENU);
    set_decorated(false);
    set_resizable(false);
    set_skip_taskbar_hint(true);
    set_skip_pager_hint(true);

    // Enable events
    add_events(Gdk::KEY_PRESS_MASK | Gdk::BUTTON_PRESS_MASK |
               Gdk::SCROLL_MASK | Gdk::FOCUS_CHANGE_MASK);
    set_can_focus(true);

    // Initialize parameters
    m_parameters = {StepParameter::FREQUENCY, StepParameter::AMPLITUDE, StepParameter::PHASE};
    m_parameter_names = {"Frequency", "Amplitude", "Phase"};

    // Set up layout
    add(m_main_box);
    m_main_box.set_margin_top(5);
    m_main_box.set_margin_bottom(5);
    m_main_box.set_margin_left(10);
    m_main_box.set_margin_right(10);

    create_parameter_labels();
    apply_styles();
    update_parameter_display();

    show_all_children();
}

void StepParameterMenu::create_parameter_labels() {
    for (std::size_t i = 0; i < m_parameters.size(); ++i) {
        auto label = new Gtk::Label();
        label->set_halign(Gtk::ALIGN_START);
        label->set_margin_top(2);
        label->set_margin_bottom(2);

        m_parameter_labels.push_back(label);
        m_main_box.pack_start(*label, Gtk::PACK_SHRINK);
    }

    update_selected_style();
}

void StepParameterMenu::update_parameter_display() {
    if (!m_controller || m_step_index >= m_controller->get_num_steps()) {
        return;
    }

    for (std::size_t i = 0; i < m_parameters.size(); ++i) {
        if (i >= m_parameter_labels.size()) continue;

        std::string value_str;
        std::ostringstream oss;

        switch (m_parameters[i]) {
            case StepParameter::FREQUENCY:
                oss << std::fixed << std::setprecision(2)
                    << m_controller->get_step_frequency(m_step_index);
                value_str = oss.str() + " Hz";
                break;

            case StepParameter::AMPLITUDE:
                oss << std::fixed << std::setprecision(3)
                    << m_controller->get_step_amplitude(m_step_index);
                value_str = oss.str();
                break;

            case StepParameter::PHASE:
                oss << std::fixed << std::setprecision(2)
                    << m_controller->get_step_phase(m_step_index);
                value_str = oss.str() + " rad";
                break;
        }

        std::string display_text = m_parameter_names[i] + ": " + value_str;

        // Show numeric input if this parameter is selected and input is active
        if (i == m_selected_parameter && m_has_numeric_input && !m_numeric_input.empty()) {
            display_text += " (" + m_numeric_input + ")";
        }

        m_parameter_labels[i]->set_text(display_text);
    }
}

void StepParameterMenu::select_parameter(std::size_t index) {
    if (index < m_parameters.size()) {
        m_selected_parameter = index;
        clear_numeric_input();
        update_selected_style();
        update_parameter_display();
    }
}

void StepParameterMenu::adjust_selected_parameter(double delta) {
    if (!m_controller || m_step_index >= m_controller->get_num_steps()) {
        return;
    }

    try {
        switch (m_parameters[m_selected_parameter]) {
            case StepParameter::FREQUENCY: {
                double current = m_controller->get_step_frequency(m_step_index);
                double new_value = std::max(0.1, current + delta * FREQUENCY_STEP);
                m_controller->set_step_frequency(m_step_index, new_value);
                break;
            }

            case StepParameter::AMPLITUDE: {
                double current = m_controller->get_step_amplitude(m_step_index);
                double new_value = std::max(0.0, std::min(1.0, current + delta * AMPLITUDE_STEP));
                m_controller->set_step_amplitude(m_step_index, new_value);
                break;
            }

            case StepParameter::PHASE: {
                double current = m_controller->get_step_phase(m_step_index);
                double new_value = current + delta * PHASE_STEP;
                // Keep phase in reasonable range
                while (new_value >= 2 * M_PI) new_value -= 2 * M_PI;
                while (new_value < 0) new_value += 2 * M_PI;
                m_controller->set_step_phase(m_step_index, new_value);
                break;
            }
        }

        clear_numeric_input();
        update_parameter_display();
    } catch (const std::exception& e) {
        std::cerr << "Error adjusting parameter: " << e.what() << std::endl;
    }
}

void StepParameterMenu::set_selected_parameter(double value) {
    if (!m_controller || m_step_index >= m_controller->get_num_steps()) {
        return;
    }

    try {
        switch (m_parameters[m_selected_parameter]) {
            case StepParameter::FREQUENCY: {
                double new_value = std::max(0.1, value);
                m_controller->set_step_frequency(m_step_index, new_value);
                break;
            }

            case StepParameter::AMPLITUDE: {
                double new_value = std::max(0.0, std::min(1.0, value));
                m_controller->set_step_amplitude(m_step_index, new_value);
                break;
            }

            case StepParameter::PHASE: {
                double new_value = value;
                // Keep phase in reasonable range
                while (new_value >= 2 * M_PI) new_value -= 2 * M_PI;
                while (new_value < 0) new_value += 2 * M_PI;
                m_controller->set_step_phase(m_step_index, new_value);
                break;
            }
        }

        update_parameter_display();
    } catch (const std::exception& e) {
        std::cerr << "Error setting parameter: " << e.what() << std::endl;
    }
}

void StepParameterMenu::handle_numeric_input(char digit) {
    m_numeric_input += digit;
    m_has_numeric_input = true;
    update_parameter_display();
}

void StepParameterMenu::apply_numeric_input() {
    if (!m_has_numeric_input || m_numeric_input.empty()) {
        return;
    }

    try {
        double value = std::stod(m_numeric_input);
        set_selected_parameter(value);
        clear_numeric_input();
    } catch (const std::exception& e) {
        std::cerr << "Invalid numeric input: " << m_numeric_input << std::endl;
        clear_numeric_input();
    }
}

void StepParameterMenu::clear_numeric_input() {
    m_numeric_input.clear();
    m_has_numeric_input = false;
}

bool StepParameterMenu::on_key_press_event(GdkEventKey* key_event) {
    // Handle Shift+Enter to close menu
    if (key_event->keyval == GDK_KEY_Return &&
        (key_event->state & GDK_SHIFT_MASK)) {
        close_menu();
        return true;
    }

    // Handle Enter to apply numeric input
    if (key_event->keyval == GDK_KEY_Return) {
        apply_numeric_input();
        return true;
    }

    // Handle Up/Down arrow keys for parameter navigation
    if (key_event->keyval == GDK_KEY_Up) {
        if (m_selected_parameter > 0) {
            select_parameter(m_selected_parameter - 1);
        }
        return true;
    }

    if (key_event->keyval == GDK_KEY_Down) {
        if (m_selected_parameter < m_parameters.size() - 1) {
            select_parameter(m_selected_parameter + 1);
        }
        return true;
    }

    // Handle Left/Right arrow keys for parameter adjustment
    if (key_event->keyval == GDK_KEY_Left) {
        adjust_selected_parameter(-1.0);
        return true;
    }

    if (key_event->keyval == GDK_KEY_Right) {
        adjust_selected_parameter(1.0);
        return true;
    }

    // Handle numeric input (0-9 and period)
    if ((key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) ||
        key_event->keyval == GDK_KEY_period) {
        char digit = static_cast<char>(key_event->keyval);
        handle_numeric_input(digit);
        return true;
    }

    // Handle Escape to close menu
    if (key_event->keyval == GDK_KEY_Escape) {
        close_menu();
        return true;
    }

    return Gtk::Window::on_key_press_event(key_event);
}

bool StepParameterMenu::on_button_press_event(GdkEventButton* button_event) {
    // Close menu if clicked outside
    close_menu();
    return true;
}

bool StepParameterMenu::on_scroll_event(GdkEventScroll* scroll_event) {
    double delta = 0.0;

    if (scroll_event->direction == GDK_SCROLL_UP) {
        delta = 1.0;
    } else if (scroll_event->direction == GDK_SCROLL_DOWN) {
        delta = -1.0;
    }

    if (delta != 0.0) {
        adjust_selected_parameter(delta);
        return true;
    }

    return Gtk::Window::on_scroll_event(scroll_event);
}

bool StepParameterMenu::on_focus_out_event(GdkEventFocus* focus_event) {
    // Close menu when focus is lost
    close_menu();
    return Gtk::Window::on_focus_out_event(focus_event);
}

void StepParameterMenu::show_at_position(int x, int y) {
    move(x, y);
    show();
    grab_focus();
    update_parameter_display();
}

void StepParameterMenu::close_menu() {
    hide();
}

void StepParameterMenu::apply_styles() {
    auto css_provider = Gtk::CssProvider::create();
    auto screen = Gdk::Screen::get_default();

    try {
        css_provider->load_from_data(
            ".parameter-menu {\n"
            "    background-color: rgba(40, 40, 40, 0.95);\n"
            "    border: 1px solid rgba(100, 100, 100, 1.0);\n"
            "    border-radius: 3px;\n"
            "}\n"
            ".parameter-unselected {\n"
            "    color: rgba(200, 200, 200, 1.0);\n"
            "    background-color: transparent;\n"
            "}\n"
            ".parameter-selected {\n"
            "    color: rgba(255, 255, 255, 1.0);\n"
            "    background-color: rgba(74, 144, 226, 0.7);\n"
            "    border-radius: 2px;\n"
            "}\n"
        );

        auto style_context = get_style_context();
        style_context->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        style_context->add_class("parameter-menu");
    } catch (const Glib::Error& e) {
        std::cerr << "Error loading parameter menu CSS: " << e.what() << std::endl;
    }
}

void StepParameterMenu::update_selected_style() {
    for (std::size_t i = 0; i < m_parameter_labels.size(); ++i) {
        auto style_context = m_parameter_labels[i]->get_style_context();

        // Remove previous classes
        style_context->remove_class("parameter-selected");
        style_context->remove_class("parameter-unselected");

        // Add appropriate class
        if (i == m_selected_parameter) {
            style_context->add_class("parameter-selected");
        } else {
            style_context->add_class("parameter-unselected");
        }
    }
}

} // namespace gui
} // namespace Micro_composer