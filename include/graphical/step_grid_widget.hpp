#ifndef MICRO_COMPOSER_GUI_STEP_GRID_WIDGET_HPP
#define MICRO_COMPOSER_GUI_STEP_GRID_WIDGET_HPP

#include <gtkmm/grid.h>
#include <vector>
#include <memory>

#include "graphical/step_box_widget.hpp"
#include "graphical/sequencer_controller.hpp"
#include "graphical/step_parameter_menu.hpp"

namespace Micro_composer {
namespace gui {

class StepGridWidget : public Gtk::Grid {
public:
    explicit StepGridWidget(std::shared_ptr<SequencerController> controller,
                           std::size_t num_steps = 8);
    virtual ~StepGridWidget() = default;

    void update_display();
    void set_current_step(std::size_t step_index);
    void clear_current_step();
    void show_parameter_menu_for_step(std::size_t step_index, int x, int y);

protected:
    void on_step_toggled(std::size_t step_index, bool active);
    void on_step_right_click(std::size_t step_index, int x, int y);

private:
    std::vector<std::unique_ptr<StepBoxWidget>> m_step_boxes;
    std::shared_ptr<SequencerController> m_controller;
    std::size_t m_num_steps;
    std::size_t m_current_step;
    bool m_has_current_step;

    void create_step_boxes();

    // Parameter menu
    std::unique_ptr<StepParameterMenu> m_parameter_menu;
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_STEP_GRID_WIDGET_HPP