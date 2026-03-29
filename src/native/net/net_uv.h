#pragma once

#include <js_engine.h>

#include <cstdint>
#include <string>

qjs::RawJSValue netConnectAsync(qjs::JSEngine& engine, int port, std::string host);
qjs::RawJSValue netWriteAsync(qjs::JSEngine& engine, int64_t sock, std::string data);
qjs::RawJSValue netReadAsync(qjs::JSEngine& engine, int64_t sock);
void netClose(int64_t sock);
