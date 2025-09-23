#ifndef MICRO_COMPOSER_GUI_CLIENT_H
#define MICRO_COMPOSER_GUI_CLIENT_H

namespace MicroComposer {
namespace client {

class GuiClient {
public:
    static int run_application(int argc, char* argv[]);

private:
    GuiClient() = default;
};

} // namespace client
} // namespace MicroComposer

#endif // MICRO_COMPOSER_GUI_CLIENT_H