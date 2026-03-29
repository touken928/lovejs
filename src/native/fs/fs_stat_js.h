#pragma once

#include <quickjs.h>
#include <uv.h>

/** Plain object shaped like Node `fs.Stats` (fields only, no methods). */
JSValue fs_stat_to_js(JSContext* c, const uv_stat_t& st);
