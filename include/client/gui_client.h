#ifndef MICRO_COMPOSER_GUI_CLIENT_H
#define MICRO_COMPOSER_GUI_CLIENT_H

namespace Micro_composer {
namespace client {

class GuiClient {
public:
  static int run_application(int argc, char* argv[]);

private:
  GuiClient() = default;
};

} // namespace client
} // namespace Micro_composer

#endif // MICRO_COMPOSER_GUI_CLIENT_H