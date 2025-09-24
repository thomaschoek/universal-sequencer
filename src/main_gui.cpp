#include "graphical/main_window.hpp"
#include "graphical/sequencer_controller.hpp"

#include <gtkmm/application.h>
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    try {
        // Create GTK application
        auto app = Gtk::Application::create(argc, argv, "com.microcomposer.stepsequencer");

        // Create the sequencer controller
        auto controller = std::make_shared<Micro_composer::gui::SequencerController>();

        // Create the main window
        Micro_composer::gui::MainWindow window(controller);

        // Run the application
        return app->run(window);

    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown application error occurred" << std::endl;
        return 1;
    }
}