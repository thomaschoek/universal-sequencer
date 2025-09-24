#include "graphical/step_grid_widget.hpp"

namespace Micro_composer {
namespace gui {

StepGridWidget::StepGridWidget(std::shared_ptr<SequencerController> controller,
                               std::size_t num_steps)
    : Gtk::Grid(), m_controller(controller), m_num_steps(num_steps),
      m_current_step(0), m_has_current_step(false) {

    // Set grid properties
    set_row_homogeneous(true);
    set_column_homogeneous(true);
    set_row_spacing(5);
    set_column_spacing(5);
    set_margin_top(10);
    set_margin_bottom(10);
    set_margin_left(10);
    set_margin_right(10);

    create_step_boxes();
}

void StepGridWidget::create_step_boxes() {
    m_step_boxes.reserve(m_num_steps);

    // Create step boxes in a single row
    for (std::size_t i = 0; i < m_num_steps; ++i) {
        auto step_box = std::make_unique<StepBoxWidget>(i);

        // Set controller for each step box
        step_box->set_controller(m_controller);

        // Connect to the step signals
        step_box->signal_step_toggled().connect(
            sigc::mem_fun(*this, &StepGridWidget::on_step_toggled));
        step_box->signal_right_click().connect(
            sigc::mem_fun(*this, &StepGridWidget::on_step_right_click));

        // Attach to grid
        attach(*step_box, static_cast<int>(i), 0, 1, 1);

        m_step_boxes.push_back(std::move(step_box));
    }

    show_all_children();
}

void StepGridWidget::on_step_toggled(std::size_t step_index, bool active) {
    if (m_controller) {
        try {
            m_controller->set_step_active(step_index, active);
        } catch (const std::exception& e) {
            // Handle error - could show a dialog or log
            // For now, just reset the button state
            if (step_index < m_step_boxes.size()) {
                m_step_boxes[step_index]->set_active(!active);
            }
        }
    }
}

void StepGridWidget::update_display() {
    if (!m_controller) {
        return;
    }

    // Update step button states based on controller
    for (std::size_t i = 0; i < m_step_boxes.size() && i < m_controller->get_num_steps(); ++i) {
        bool should_be_active = m_controller->is_step_active(i);
        if (m_step_boxes[i]->get_active() != should_be_active) {
            m_step_boxes[i]->set_active(should_be_active);
        }
    }

    // Update current step highlighting
    if (m_controller->has_current_step()) {
        std::size_t current = m_controller->get_current_step();
        set_current_step(current);
    } else {
        clear_current_step();
    }
}

void StepGridWidget::set_current_step(std::size_t step_index) {
    // Clear previous current step
    if (m_has_current_step && m_current_step < m_step_boxes.size()) {
        m_step_boxes[m_current_step]->set_current(false);
    }

    // Set new current step
    if (step_index < m_step_boxes.size()) {
        m_step_boxes[step_index]->set_current(true);
        m_current_step = step_index;
        m_has_current_step = true;
    }
}

void StepGridWidget::clear_current_step() {
    if (m_has_current_step && m_current_step < m_step_boxes.size()) {
        m_step_boxes[m_current_step]->set_current(false);
        m_has_current_step = false;
    }
}

void StepGridWidget::on_step_right_click(std::size_t step_index, int x, int y) {
    show_parameter_menu_for_step(step_index, x, y);
}

void StepGridWidget::show_parameter_menu_for_step(std::size_t step_index, int x, int y) {
    if (!m_controller) {
        return;
    }

    // Close any existing menu
    if (m_parameter_menu) {
        m_parameter_menu->close_menu();
    }

    // Create new parameter menu for this step
    m_parameter_menu = std::make_unique<StepParameterMenu>(m_controller, step_index);

    // Show menu at the clicked position
    m_parameter_menu->show_at_position(x, y);
}

} // namespace gui
} // namespace Micro_composer