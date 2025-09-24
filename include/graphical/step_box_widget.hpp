#ifndef MICRO_COMPOSER_GUI_STEP_BOX_WIDGET_HPP
#define MICRO_COMPOSER_GUI_STEP_BOX_WIDGET_HPP

#include <gtkmm/togglebutton.h>
#include <gtkmm/stylecontext.h>
#include <sigc++/sigc++.h>

namespace Micro_composer {
namespace gui {

class StepBoxWidget : public Gtk::ToggleButton {
public:
    explicit StepBoxWidget(std::size_t step_index);
    virtual ~StepBoxWidget() = default;

    void set_current(bool is_current);
    bool get_current() const { return m_is_current; }

    std::size_t get_step_index() const { return m_step_index; }

    // Signal accessor
    typedef sigc::signal<void(std::size_t, bool)> SignalStepToggled;
    SignalStepToggled signal_step_toggled() { return m_signal_step_toggled; }

protected:
    void on_toggled() override;
    void update_style();

private:
    std::size_t m_step_index;
    bool m_is_current;
    SignalStepToggled m_signal_step_toggled;
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_STEP_BOX_WIDGET_HPP