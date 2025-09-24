#include "graphical/step_box_widget.hpp"
#include <iostream>

namespace Micro_composer {
namespace gui {

StepBoxWidget::StepBoxWidget(std::size_t step_index)
    : Gtk::ToggleButton(), m_step_index(step_index), m_is_current(false) {

    // Set widget properties
    set_size_request(60, 60);
    set_label(std::to_string(step_index + 1)); // 1-indexed display

    // Enable button press events for right-click detection
    add_events(Gdk::BUTTON_PRESS_MASK);

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

void StepBoxWidget::set_controller(std::shared_ptr<SequencerController> controller) {
    m_controller = controller;
}

bool StepBoxWidget::on_button_press_event(GdkEventButton* button_event) {
    // Handle right-click
    if (button_event->button == GDK_BUTTON_SECONDARY) {
        // Get widget position to show menu at the right location
        int widget_x, widget_y;
        get_window()->get_origin(widget_x, widget_y);

        // Add widget-relative coordinates
        widget_x += static_cast<int>(button_event->x);
        widget_y += static_cast<int>(button_event->y);

        // Emit right-click signal with screen coordinates
        m_signal_right_click.emit(m_step_index, widget_x, widget_y);
        return true; // Event handled
    }

    // Let the base class handle other button events (left-click toggle)
    return Gtk::ToggleButton::on_button_press_event(button_event);
}

} // namespace gui
} // namespace Micro_composer