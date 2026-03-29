#pragma once

#include <js_engine.h>
#include <string>

qjs::RawJSValue fsReadFileAsync(qjs::JSEngine& engine, std::string path);
qjs::RawJSValue fsWriteFileAsync(qjs::JSEngine& engine, std::string path, std::string data);
