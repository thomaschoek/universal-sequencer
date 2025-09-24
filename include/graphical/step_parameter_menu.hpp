#ifndef MICRO_COMPOSER_GUI_STEP_PARAMETER_MENU_HPP
#define MICRO_COMPOSER_GUI_STEP_PARAMETER_MENU_HPP

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <memory>
#include <vector>
#include <string>

#include "graphical/sequencer_controller.hpp"

namespace Micro_composer {
namespace gui {

enum class StepParameter {
    FREQUENCY,
    AMPLITUDE,
    PHASE
};

class StepParameterMenu : public Gtk::Window {
public:
    explicit StepParameterMenu(std::shared_ptr<SequencerController> controller,
                               std::size_t step_index);
    virtual ~StepParameterMenu() = default;

    void show_at_position(int x, int y);
    void close_menu();

protected:
    bool on_key_press_event(GdkEventKey* key_event) override;
    bool on_button_press_event(GdkEventButton* button_event) override;
    bool on_scroll_event(GdkEventScroll* scroll_event) override;
    bool on_focus_out_event(GdkEventFocus* focus_event) override;

private:
    void create_parameter_labels();
    void update_parameter_display();
    void select_parameter(std::size_t index);
    void adjust_selected_parameter(double delta);
    void set_selected_parameter(double value);
    void handle_numeric_input(char digit);
    void apply_numeric_input();
    void clear_numeric_input();

    std::shared_ptr<SequencerController> m_controller;
    std::size_t m_step_index;
    std::size_t m_selected_parameter;

    // GUI components
    Gtk::Box m_main_box;
    std::vector<Gtk::Label*> m_parameter_labels;

    // Parameter data
    std::vector<StepParameter> m_parameters;
    std::vector<std::string> m_parameter_names;

    // Numeric input state
    std::string m_numeric_input;
    bool m_has_numeric_input;

    // Styling
    void apply_styles();
    void update_selected_style();

    // Constants
    static constexpr double FREQUENCY_STEP = 10.0;
    static constexpr double AMPLITUDE_STEP = 0.05;
    static constexpr double PHASE_STEP = 0.1;
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_STEP_PARAMETER_MENU_HPP