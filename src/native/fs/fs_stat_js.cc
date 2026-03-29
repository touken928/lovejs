#include "native/fs/fs_stat_js.h"

#include <cstdint>

#include <sys/stat.h>

static int64_t timespec_to_ms(const uv_timespec_t& t) {
    return static_cast<int64_t>(t.tv_sec) * 1000 + static_cast<int64_t>(t.tv_nsec) / 1000000;
}

JSValue fs_stat_to_js(JSContext* c, const uv_stat_t& st) {
    JSValue o = JS_NewObject(c);
    if (JS_IsException(o))
        return o;

    auto def = [&](const char* name, JSValue v) {
        if (JS_IsException(v) || JS_DefinePropertyValueStr(c, o, name, v, JS_PROP_C_W_E) < 0) {
            JS_FreeValue(c, o);
            return false;
        }
        return true;
    };

#ifndef S_IFMT
#define S_IFMT 0170000
#endif
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif
#ifndef S_IFIFO
#define S_IFIFO 0010000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

    const uint64_t mode = st.st_mode;
    const bool is_file = (mode & S_IFMT) == S_IFREG;
    const bool is_dir = (mode & S_IFMT) == S_IFDIR;
    const bool is_symlink = (mode & S_IFMT) == S_IFLNK;
    const bool is_chr = (mode & S_IFMT) == S_IFCHR;
    const bool is_blk = (mode & S_IFMT) == S_IFBLK;
    const bool is_fifo = (mode & S_IFMT) == S_IFIFO;
    const bool is_sock = (mode & S_IFMT) == S_IFSOCK;

    if (!def("dev", JS_NewInt64(c, static_cast<int64_t>(st.st_dev)))
        || !def("ino", JS_NewInt64(c, static_cast<int64_t>(st.st_ino)))
        || !def("mode", JS_NewInt64(c, static_cast<int64_t>(st.st_mode)))
        || !def("nlink", JS_NewInt64(c, static_cast<int64_t>(st.st_nlink)))
        || !def("uid", JS_NewInt64(c, static_cast<int64_t>(st.st_uid)))
        || !def("gid", JS_NewInt64(c, static_cast<int64_t>(st.st_gid)))
        || !def("rdev", JS_NewInt64(c, static_cast<int64_t>(st.st_rdev)))
        || !def("size", JS_NewInt64(c, static_cast<int64_t>(st.st_size)))
        || !def("blksize", JS_NewInt64(c, static_cast<int64_t>(st.st_blksize)))
        || !def("blocks", JS_NewInt64(c, static_cast<int64_t>(st.st_blocks)))
        || !def("atimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_atim))))
        || !def("mtimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_mtim))))
        || !def("ctimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_ctim))))
        || !def("birthtimeMs", JS_NewFloat64(c, static_cast<double>(timespec_to_ms(st.st_birthtim))))
        || !def("isFile", JS_NewBool(c, is_file))
        || !def("isDirectory", JS_NewBool(c, is_dir))
        || !def("isSymbolicLink", JS_NewBool(c, is_symlink))
        || !def("isCharacterDevice", JS_NewBool(c, is_chr))
        || !def("isBlockDevice", JS_NewBool(c, is_blk))
        || !def("isFIFO", JS_NewBool(c, is_fifo))
        || !def("isSocket", JS_NewBool(c, is_sock))) {
        return JS_EXCEPTION;
    }

    return o;
}
