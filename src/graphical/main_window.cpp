#include "graphical/main_window.h"
#include "graphical/gui_application.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

// Simple cross-platform console-based UI for now
// This can be replaced with a proper GUI framework later

namespace Micro_composer {
namespace gui {

struct MainWindow::Impl {
  bool window_open = true;
  bool needs_refresh = true;
  int selected_option = 0;

  void display_interface() {
    if (!needs_refresh)
      return;

    system("clear"); // Clear console (Linux/Mac)

    std::cout << "=== Micro_composer GUI ===\n\n";
    std::cout << "Current sequence: C major scale (8 steps)\n";
    std::cout << "Current tempo: 120 BPM\n\n";
    std::cout << "Controls:\n";
    std::cout << "1. Play/Start Sequencer\n";
    std::cout << "2. Stop Sequencer\n";
    std::cout << "3. Change Tempo\n";
    std::cout << "4. Edit Note Frequencies (0-7)\n";
    std::cout << "5. Add New Event\n";
    std::cout << "6. Insert Event at Position\n";
    std::cout << "7. Remove Event\n";
    std::cout << "8. Remove Events Range\n";
    std::cout << "9. Exit\n\n";
    std::cout << "Enter your choice (1-9): ";
    std::cout.flush();

    needs_refresh = false;
  }

  int get_user_input() {
    int choice;
    if (std::scanf("%d", &choice) == 1) {
      return choice;
    }
    return 0;
  }

  void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
      ;
  }
};

MainWindow::MainWindow(GuiApplication* app)
    : pimpl_(std::make_unique<Impl>()), app_(app) {}

MainWindow::~MainWindow() = default;

void MainWindow::show() {
  std::cout << "Micro_composer GUI Started\n";
  std::cout << "Console-based interface active\n\n";
}

void MainWindow::hide() { pimpl_->window_open = false; }

bool MainWindow::process_events() {
  if (!pimpl_->window_open) {
    return false;
  }

  pimpl_->display_interface();

  int choice = pimpl_->get_user_input();
  pimpl_->clear_input_buffer();
  pimpl_->needs_refresh = true;

  switch (choice) {
  case 1:
    if (on_play_clicked) {
      on_play_clicked();
    }
    break;

  case 2:
    if (on_stop_clicked) {
      on_stop_clicked();
    }
    break;

  case 3: {
    std::cout << "Enter new tempo (BPM): ";
    double tempo;
    if (std::scanf("%lf", &tempo) == 1) {
      if (tempo > 0) {
        if (on_tempo_changed) {
          on_tempo_changed(tempo);
        }
      } else {
        std::cout << "Invalid tempo value (must be > 0)\n";
      }
    } else {
      std::cout << "Invalid input for tempo\n";
    }
    pimpl_->clear_input_buffer();
    break;
  }

  case 4: {
    std::cout << "Enter step number (0-7): ";
    int step;
    if (std::scanf("%d", &step) == 1) {
      if (step >= 0 && step <= 7) {
        std::cout << "Enter frequency (Hz): ";
        double freq;
        if (std::scanf("%lf", &freq) == 1 && freq > 0) {
          if (on_note_changed) {
            on_note_changed(step, freq);
          }
        } else {
          std::cout << "Invalid frequency value\n";
        }
      } else {
        std::cout << "Invalid step number\n";
      }
    } else {
      std::cout << "Invalid input for step number\n";
    }
    pimpl_->clear_input_buffer(); // Clear at the end only
    break;
  }

  case 5: {
    std::cout << "Enter frequency (Hz): ";
    double freq;
    if (std::scanf("%lf", &freq) == 1 && freq > 0) {
      pimpl_->clear_input_buffer();
      std::cout << "Enter amplitude (0.0-1.0): ";
      double amp;
      if (std::scanf("%lf", &amp) == 1 && amp >= 0 && amp <= 1) {
        pimpl_->clear_input_buffer();
        std::cout << "Enter phase (0.0-1.0): ";
        double phase;
        if (std::scanf("%lf", &phase) == 1 && phase >= 0 && phase <= 1) {
          if (on_add_event) {
            on_add_event(freq, amp, phase);
          }
        } else {
          std::cout << "Invalid phase value\n";
        }
      } else {
        std::cout << "Invalid amplitude value\n";
      }
    } else {
      std::cout << "Invalid frequency value\n";
    }
    pimpl_->clear_input_buffer();
    break;
  }

  case 6: {
    std::cout << "Enter position to insert at: ";
    int pos;
    std::cout << "Enter frequency (Hz): ";
    double freq;
    std::cout << "Enter amplitude (0.0-1.0): ";
    double amp;
    std::cout << "Enter phase (0.0-1.0): ";
    double phase;
    if (std::scanf("%d", &pos) == 1 && pos >= 0 &&
        std::scanf("%lf", &freq) == 1 && freq > 0 &&
        std::scanf("%lf", &amp) == 1 && amp >= 0 && amp <= 1 &&
        std::scanf("%lf", &phase) == 1 && phase >= 0 && phase <= 1) {
      if (on_insert_event) {
        on_insert_event(pos, freq, amp, phase);
      }
    } else {
      std::cout << "Invalid values\n";
    }
    pimpl_->clear_input_buffer();
    break;
  }

  case 7: {
    std::cout << "Enter position to remove: ";
    int pos;
    if (std::scanf("%d", &pos) == 1 && pos >= 0) {
      if (on_remove_event) {
        on_remove_event(pos);
      }
    } else {
      std::cout << "Invalid position\n";
    }
    pimpl_->clear_input_buffer();
    break;
  }

  case 8: {
    std::cout << "Enter first position: ";
    int first;
    std::cout << "Enter last position: ";
    int last;
    if (std::scanf("%d", &first) == 1 && first >= 0 &&
        std::scanf("%d", &last) == 1 && last > first) {
      if (on_remove_events_range) {
        on_remove_events_range(first, last);
      }
    } else {
      std::cout << "Invalid range\n";
    }
    pimpl_->clear_input_buffer();
    break;
  }

  case 9:
    std::cout << "Exiting Micro_composer...\n";
    return false;

  default:
    std::cout << "Invalid choice. Please select 1-9.\n";
    break;
  }

  return true;
}

void MainWindow::create_controls() {
  // Console interface doesn't need explicit control creation
}

void MainWindow::update_play_button(bool is_playing) {
  // Update will happen on next refresh
  pimpl_->needs_refresh = true;
}

} // namespace gui
} // namespace Micro_composer