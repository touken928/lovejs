#pragma once

#include <js_engine.h>

#include <string>
#include <vector>

qjs::RawJSValue fsReadFileAsync(qjs::JSEngine& engine, std::string path, bool asBuffer);

qjs::RawJSValue fsWriteFileAsync(qjs::JSEngine& engine, std::string path, std::vector<uint8_t> data);

qjs::RawJSValue fsMkdirAsync(qjs::JSEngine& engine, std::string path, bool recursive);

qjs::RawJSValue fsReaddirAsync(qjs::JSEngine& engine, std::string path);

qjs::RawJSValue fsStatAsync(qjs::JSEngine& engine, std::string path);

qjs::RawJSValue fsUnlinkAsync(qjs::JSEngine& engine, std::string path);

qjs::RawJSValue fsRmdirAsync(qjs::JSEngine& engine, std::string path);
