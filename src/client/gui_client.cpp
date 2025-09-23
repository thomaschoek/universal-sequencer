#include "client/gui_client.h"
#include "graphical/gui_application.h"

#include <iostream>
#include <exception>

namespace MicroComposer {
namespace client {

int GuiClient::run_application(int argc, char* argv[]) {
    try {
        gui::GuiApplication app;
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error running GUI application: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error running GUI application" << std::endl;
        return 1;
    }
}

} // namespace client
} // namespace MicroComposer