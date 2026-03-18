// Backward-compatible shim:
// Old code includes "core/JSEngine.hpp".
// JS engine implementation was extracted into /jsengine.
#pragma once
#include <slowjs/JSEngine.hpp>

// Keep legacy name in global namespace for minimal churn in existing code.
using JSEngine = slowjs::JSEngine;
using JSModule = slowjs::JSModule;
