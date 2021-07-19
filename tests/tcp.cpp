#include <telesto/uv_loop.hpp>
#include <telesto/net/tcp.hpp>
#include <async/result.hpp>

int main() {
	async::run_queue rq;
	async::queue_scope qs{&rq};
	tlst::uv_loop ul;
	tlst::uv_loop_service sv{&ul};

	async::run([&ul] () -> async::result<void> {
		tlst::tcp_server svr{&ul};
		svr.bind_v4("0.0.0.0", 1234);
		svr.listen();

		while (true) {
			auto conn = co_await svr.accept();
			async::detach([] (auto conn) -> async::result<void> {
				printf("conn %p\n", conn.get());

				if (auto buf = co_await conn->recv()) {
					printf("%.*s\n", buf->size(), buf->data());
					co_await conn->send(std::as_bytes(std::span{"HTTP/1.1 200 OK\r\nServer: test\r\nContent-Type: text/html\r\nContent-Length: 21\r\n\r\n<h1>Hello world!</h1>"}));
				}

				printf("dc\n");
			}(std::move(conn)));
		}

	}(), async::current_queue, sv);
}
