#include "native/net/net_uv.h"

#include "runtime/event_loop/event_loop.h"

#include <uvw.hpp>

#include <atomic>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

class SocketCtx;

std::mutex g_sock_mu;
std::unordered_map<int64_t, SocketCtx*> g_sockets;
std::atomic<int64_t> g_next_id{1};

void schedule_reject(qjs::JSEngine::PromiseHandle ph, std::string msg) {
    qianjs::event_loop::defer(
        [ph, msg = std::move(msg)](qjs::JSEngine& e) {
            e.rejectPromise(ph, msg);
            e.freePromise(ph);
        });
}

void schedule_resolve_string(qjs::JSEngine::PromiseHandle ph, std::string s) {
    qianjs::event_loop::defer(
        [ph, s = std::move(s)](qjs::JSEngine& e) {
            e.resolvePromise(ph, s);
            e.freePromise(ph);
        });
}

void schedule_resolve_void(qjs::JSEngine::PromiseHandle ph) {
    qianjs::event_loop::defer([ph](qjs::JSEngine& e) {
        e.resolvePromiseVoid(ph);
        e.freePromise(ph);
    });
}

void schedule_resolve_int64(qjs::JSEngine::PromiseHandle ph, int64_t n) {
    qianjs::event_loop::defer([ph, n](qjs::JSEngine& e) {
        e.resolvePromise(ph, n);
        e.freePromise(ph);
    });
}

struct WriteOpCtx {
    qjs::JSEngine::PromiseHandle ph{};
    std::string bytes;
};

class SocketCtx {
public:
    int64_t id = 0;
    std::shared_ptr<uvw::tcp_handle> tcp;

    std::string inbound;
    qjs::JSEngine::PromiseHandle read_ph{};
    bool read_wait = false;
    bool read_started = false;
    bool eof = false;
    bool closing = false;
    bool connected = false;

    qjs::JSEngine::PromiseHandle connect_ph{};

    std::deque<WriteOpCtx*> write_queue;
    bool write_busy = false;
    WriteOpCtx* write_inflight = nullptr;

    void fulfill_read(std::string chunk) {
        if (!read_wait || !read_ph.ptr)
            return;
        auto ph = read_ph;
        read_ph = {};
        read_wait = false;
        qianjs::event_loop::end_operation();
        schedule_resolve_string(ph, std::move(chunk));
    }

    void try_deliver_read() {
        if (read_wait && !inbound.empty()) {
            std::string chunk = std::move(inbound);
            inbound.clear();
            fulfill_read(std::move(chunk));
        } else if (read_wait && eof) {
            fulfill_read({});
        }
    }

    void pump_writes() {
        if (write_busy || write_queue.empty() || closing)
            return;
        WriteOpCtx* op = write_queue.front();
        write_queue.pop_front();
        write_busy = true;
        write_inflight = op;

        const unsigned len = static_cast<unsigned>(op->bytes.size());
        std::unique_ptr<char[]> buf;
        if (len == 0) {
            buf = std::make_unique<char[]>(1);
        } else {
            buf = std::make_unique<char[]>(op->bytes.size());
            std::memcpy(buf.get(), op->bytes.data(), op->bytes.size());
        }

        if (tcp->write(std::move(buf), len) != 0) {
            write_busy = false;
            write_inflight = nullptr;
            qianjs::event_loop::end_operation();
            auto ph = op->ph;
            delete op;
            schedule_reject(ph, "uv_write failed");
            pump_writes();
        }
    }
};

static void wire_socket(SocketCtx* sock) {
    auto t = sock->tcp;

    t->on<uvw::close_event>([sock](const uvw::close_event&, auto&) { delete sock; });

    t->on<uvw::connect_event>([sock](const uvw::connect_event&, auto&) {
        sock->connected = true;
        {
            std::lock_guard<std::mutex> lock(g_sock_mu);
            g_sockets[sock->id] = sock;
        }
        qianjs::event_loop::end_operation();
        auto ph = sock->connect_ph;
        sock->connect_ph = {};
        schedule_resolve_int64(ph, sock->id);
    });

    t->on<uvw::write_event>([sock](const uvw::write_event&, auto&) {
        if (!sock->write_busy || !sock->write_inflight)
            return;
        WriteOpCtx* op = sock->write_inflight;
        sock->write_inflight = nullptr;
        sock->write_busy = false;
        qianjs::event_loop::end_operation();
        auto ph = op->ph;
        delete op;
        schedule_resolve_void(ph);
        sock->pump_writes();
    });

    t->on<uvw::data_event>([sock](const uvw::data_event& ev, auto&) {
        if (ev.length && ev.data)
            sock->inbound.append(ev.data.get(), ev.length);
        sock->try_deliver_read();
    });

    t->on<uvw::end_event>([sock](const uvw::end_event&, auto&) {
        sock->eof = true;
        sock->tcp->stop();
        sock->try_deliver_read();
    });

    t->on<uvw::error_event>([sock](const uvw::error_event& e, auto&) {
        if (!sock->connected) {
            qianjs::event_loop::end_operation();
            auto ph = sock->connect_ph;
            sock->connect_ph = {};
            schedule_reject(ph, e.what());
            sock->tcp->close();
            return;
        }
        if (sock->write_busy && sock->write_inflight) {
            WriteOpCtx* op = sock->write_inflight;
            sock->write_inflight = nullptr;
            sock->write_busy = false;
            qianjs::event_loop::end_operation();
            auto ph = op->ph;
            delete op;
            schedule_reject(ph, e.what());
            sock->pump_writes();
            return;
        }
        if (sock->read_wait && sock->read_ph.ptr) {
            auto ph = sock->read_ph;
            sock->read_ph = {};
            sock->read_wait = false;
            qianjs::event_loop::end_operation();
            schedule_reject(ph, e.what());
        }
    });
}

SocketCtx* lookup_unlocked(int64_t id) {
    auto it = g_sockets.find(id);
    return it == g_sockets.end() ? nullptr : it->second;
}

} // namespace

qjs::RawJSValue netConnectAsync(qjs::JSEngine& engine, int port, std::string host) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    if (port <= 0 || port > 65535) {
        schedule_reject(ph, "invalid port");
        return engine.promiseValue(ph);
    }
    if (host.empty())
        host = "127.0.0.1";

    auto loop = qianjs::event_loop::uv::uvw_loop();

    struct DnsHold {
        std::shared_ptr<uvw::get_addr_info_req> req;
    };
    auto dns_hold = std::make_shared<DnsHold>();
    dns_hold->req = loop->resource<uvw::get_addr_info_req>();

    auto finish_dns = [dns_hold] {
        qianjs::event_loop::defer([dns_hold](qjs::JSEngine&) { dns_hold->req.reset(); });
    };

    dns_hold->req->on<uvw::error_event>([ph, finish_dns](const uvw::error_event& e, auto&) {
        qianjs::event_loop::end_operation();
        schedule_reject(ph, e.what());
        finish_dns();
    });

    dns_hold->req->on<uvw::addr_info_event>([ph, loop, finish_dns](const uvw::addr_info_event& ev, auto&) {
        struct addrinfo* p = ev.data.get();
        while (p && p->ai_family != AF_INET && p->ai_family != AF_INET6)
            p = p->ai_next;
        if (!p) {
            qianjs::event_loop::end_operation();
            schedule_reject(ph, "no address");
            finish_dns();
            return;
        }

        sockaddr_storage storage{};
        std::memcpy(&storage, p->ai_addr, p->ai_addrlen);

        auto* sock = new SocketCtx();
        sock->id = g_next_id.fetch_add(1, std::memory_order_relaxed);
        sock->tcp = loop->resource<uvw::tcp_handle>();
        sock->connect_ph = ph;

        if (sock->tcp->init() != 0) {
            qianjs::event_loop::end_operation();
            schedule_reject(ph, "uv_tcp_init failed");
            delete sock;
            finish_dns();
            return;
        }

        wire_socket(sock);

        const auto* addr = reinterpret_cast<const sockaddr*>(static_cast<void*>(&storage));
        if (sock->tcp->connect(*addr) != 0) {
            qianjs::event_loop::end_operation();
            schedule_reject(ph, "uv_tcp_connect failed");
            sock->tcp->close();
            finish_dns();
            return;
        }
        finish_dns();
    });

    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    qianjs::event_loop::begin_operation();
    dns_hold->req->addr_info(host, std::to_string(port), &hints);
    return engine.promiseValue(ph);
}

qjs::RawJSValue netWriteAsync(qjs::JSEngine& engine, int64_t sock_id, std::string data) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    SocketCtx* s = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_sock_mu);
        s = lookup_unlocked(sock_id);
    }
    if (!s || s->closing) {
        schedule_reject(ph, "invalid socket");
        return engine.promiseValue(ph);
    }

    auto* op = new WriteOpCtx{};
    op->ph = ph;
    op->bytes = std::move(data);
    qianjs::event_loop::begin_operation();
    s->write_queue.push_back(op);
    s->pump_writes();
    return engine.promiseValue(ph);
}

qjs::RawJSValue netReadAsync(qjs::JSEngine& engine, int64_t sock_id) {
    qjs::JSEngine::PromiseHandle ph = engine.createPromise();
    if (!ph.ptr)
        return engine.promiseValue(ph);

    SocketCtx* s = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_sock_mu);
        s = lookup_unlocked(sock_id);
    }
    if (!s || s->closing) {
        schedule_reject(ph, "invalid socket");
        return engine.promiseValue(ph);
    }
    if (s->read_wait) {
        schedule_reject(ph, "read already pending");
        return engine.promiseValue(ph);
    }

    if (!s->inbound.empty()) {
        std::string chunk = std::move(s->inbound);
        s->inbound.clear();
        schedule_resolve_string(ph, std::move(chunk));
        return engine.promiseValue(ph);
    }
    if (s->eof) {
        schedule_resolve_string(ph, {});
        return engine.promiseValue(ph);
    }

    s->read_wait = true;
    s->read_ph = ph;
    qianjs::event_loop::begin_operation();
    if (!s->read_started) {
        if (s->tcp->read() != 0) {
            s->read_wait = false;
            s->read_ph = {};
            qianjs::event_loop::end_operation();
            schedule_reject(ph, "uv_read_start failed");
            return engine.promiseValue(ph);
        }
        s->read_started = true;
    }
    return engine.promiseValue(ph);
}

void netClose(int64_t sock_id) {
    SocketCtx* s = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_sock_mu);
        s = lookup_unlocked(sock_id);
        if (s)
            g_sockets.erase(sock_id);
    }
    if (!s)
        return;
    s->closing = true;
    if (s->read_wait && s->read_ph.ptr) {
        auto ph = s->read_ph;
        s->read_ph = {};
        s->read_wait = false;
        qianjs::event_loop::end_operation();
        schedule_reject(ph, "socket closed");
    }
    while (!s->write_queue.empty()) {
        WriteOpCtx* op = s->write_queue.front();
        s->write_queue.pop_front();
        qianjs::event_loop::end_operation();
        schedule_reject(op->ph, "socket closed");
        delete op;
    }
    if (s->write_inflight) {
        WriteOpCtx* op = s->write_inflight;
        s->write_inflight = nullptr;
        s->write_busy = false;
        qianjs::event_loop::end_operation();
        schedule_reject(op->ph, "socket closed");
        delete op;
    }
    if (s->read_started)
        s->tcp->stop();
    s->tcp->close();
}
