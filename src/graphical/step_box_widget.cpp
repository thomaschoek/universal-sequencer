#include "graphical/step_box_widget.hpp"

namespace Micro_composer {
namespace gui {

StepBoxWidget::StepBoxWidget(std::size_t step_index)
    : Gtk::ToggleButton(), m_step_index(step_index), m_is_current(false) {

    // Set widget properties
    set_size_request(60, 60);
    set_label(std::to_string(step_index + 1)); // 1-indexed display

    // Set initial styling for dark theme
    update_style();

    // Connect signals
    signal_toggled().connect(sigc::mem_fun(*this, &StepBoxWidget::on_toggled));
}

void StepBoxWidget::set_current(bool is_current) {
    if (m_is_current != is_current) {
        m_is_current = is_current;
        update_style();
    }
}

void StepBoxWidget::on_toggled() {
    // Emit our custom signal
    m_signal_step_toggled.emit(m_step_index, get_active());
    update_style();
}

void StepBoxWidget::update_style() {
    auto context = get_style_context();

    // Remove all previous style classes
    context->remove_class("step-inactive");
    context->remove_class("step-active");
    context->remove_class("step-current");
    context->remove_class("step-current-active");

    // Apply appropriate style class based on state
    if (m_is_current) {
        if (get_active()) {
            context->add_class("step-current-active");
        } else {
            context->add_class("step-current");
        }
    } else {
        if (get_active()) {
            context->add_class("step-active");
        } else {
            context->add_class("step-inactive");
        }
    }
}

} // namespace gui
} // namespace Micro_composer