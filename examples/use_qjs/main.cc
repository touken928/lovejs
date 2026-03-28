#include <js_engine.h>

int main() {
    qjs::JSEngine engine;
    engine.initialize();
    engine.cleanup();
    return 0;
}
