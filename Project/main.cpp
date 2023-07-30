#include "Application.h"

int main() {
    Application app;
    try {
        app.Run();
    } catch (std::exception const &e) {
        std::printf("Unhandled exception: %s\n", e.what());
    }
    return 0;
}