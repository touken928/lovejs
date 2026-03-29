#pragma once

#include <js_engine.h>

#include <cstdint>
#include <string>
#include <vector>

qjs::RawJSValue netConnectAsync(qjs::JSEngine& engine, int port, std::string host);
qjs::RawJSValue netWriteAsync(qjs::JSEngine& engine, int64_t sock, std::vector<uint8_t> data);
qjs::RawJSValue netReadAsync(qjs::JSEngine& engine, int64_t sock);
qjs::RawJSValue netReadBytesAsync(qjs::JSEngine& engine, int64_t sock);
void netClose(int64_t sock);
