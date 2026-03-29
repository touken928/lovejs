#pragma once

#include <js_engine.h>

#include <cstdint>
#include <string>
#include <vector>

qjs::RawJSValue fsReadFileAsync(qjs::JSEngine& engine, std::string path, bool asBuffer, std::string flag);

qjs::RawJSValue fsWriteFileAsync(qjs::JSEngine& engine, std::string path, std::vector<uint8_t> data, std::string flag,
    int mode);

qjs::RawJSValue fsAppendFileAsync(qjs::JSEngine& engine, std::string path, std::vector<uint8_t> data, int mode);

qjs::RawJSValue fsMkdirAsync(qjs::JSEngine& engine, std::string path, bool recursive, int mode);

qjs::RawJSValue fsReaddirAsync(qjs::JSEngine& engine, std::string path);

qjs::RawJSValue fsStatAsync(qjs::JSEngine& engine, std::string path, bool followSymlink);

qjs::RawJSValue fsUnlinkAsync(qjs::JSEngine& engine, std::string path);

qjs::RawJSValue fsRmdirAsync(qjs::JSEngine& engine, std::string path);

qjs::RawJSValue fsRenameAsync(qjs::JSEngine& engine, std::string oldPath, std::string newPath);

qjs::RawJSValue fsCopyFileAsync(qjs::JSEngine& engine, std::string src, std::string dest, int copyFlags);
