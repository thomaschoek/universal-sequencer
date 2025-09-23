#ifndef MICRO_COMPOSER_MAIN_WINDOW_H
#define MICRO_COMPOSER_MAIN_WINDOW_H

#include <functional>
#include <memory>
#include <vector>

namespace Micro_composer {
namespace gui {

class GuiApplication;

class MainWindow {
public:
  explicit MainWindow(GuiApplication* app);
  ~MainWindow();

  void show();
  void hide();
  bool process_events();

  // Callbacks for GUI events
  std::function<void()> on_play_clicked;
  std::function<void()> on_stop_clicked;
  std::function<void(double)> on_tempo_changed;
  std::function<void(int, double)> on_note_changed;

private:
  void create_controls();
  void update_play_button(bool is_playing);

  struct Impl;
  std::unique_ptr<Impl> pimpl_;
  GuiApplication* app_;
};

} // namespace gui
} // namespace Micro_composer

#endif // MICRO_COMPOSER_MAIN_WINDOW_H