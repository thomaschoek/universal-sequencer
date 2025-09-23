#include "graphical/main_window.h"
#include "graphical/gui_application.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>

// Simple cross-platform console-based UI for now
// This can be replaced with a proper GUI framework later

namespace MicroComposer {
namespace gui {

struct MainWindow::Impl {
    bool window_open = true;
    bool needs_refresh = true;
    int selected_option = 0;

    void display_interface() {
        if (!needs_refresh) return;

        system("clear"); // Clear console (Linux/Mac)

        std::cout << "=== MicroComposer GUI ===\n\n";
        std::cout << "Current sequence: C major scale (8 steps)\n";
        std::cout << "Current tempo: 120 BPM\n\n";
        std::cout << "Controls:\n";
        std::cout << "1. Play/Start Sequencer\n";
        std::cout << "2. Stop Sequencer\n";
        std::cout << "3. Change Tempo\n";
        std::cout << "4. Edit Note Frequencies (0-7)\n";
        std::cout << "5. Exit\n\n";
        std::cout << "Enter your choice (1-5): ";
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
        while ((c = getchar()) != '\n' && c != EOF);
    }
};

MainWindow::MainWindow(GuiApplication* app)
    : pimpl_(std::make_unique<Impl>())
    , app_(app) {
}

MainWindow::~MainWindow() = default;

void MainWindow::show() {
    std::cout << "MicroComposer GUI Started\n";
    std::cout << "Console-based interface active\n\n";
}

void MainWindow::hide() {
    pimpl_->window_open = false;
}

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
            if (std::scanf("%lf", &tempo) == 1 && tempo > 0) {
                if (on_tempo_changed) {
                    on_tempo_changed(tempo);
                }
            } else {
                std::cout << "Invalid tempo value\n";
            }
            pimpl_->clear_input_buffer();
            break;
        }

        case 4: {
            std::cout << "Enter step number (0-7): ";
            int step;
            if (std::scanf("%d", &step) == 1 && step >= 0 && step <= 7) {
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
            pimpl_->clear_input_buffer();
            break;
        }

        case 5:
            std::cout << "Exiting MicroComposer...\n";
            return false;

        default:
            std::cout << "Invalid choice. Please select 1-5.\n";
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
} // namespace MicroComposer