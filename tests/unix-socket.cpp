#include <telesto/platform/poll/poll_poll_engine.hpp>
#include <telesto/platform/default_poll_engine.hpp>
#include <telesto/async/io_service.hpp>
#include <telesto/io/io_socket.hpp>
#include <async/result.hpp>
#include <sys/socket.h>

int main() {
	async::run_queue rq;
	async::queue_scope qs{&rq};
	//tlst::io_service ios{new tlst::platform::poll_poll_engine};
	tlst::io_service ios;

	async::run([&ios] () -> async::result<void> {
		int fds[2];
		assert(!socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds));

		ios.add(new tlst::io_socket{&ios, fds[0]});
		ios.add(new tlst::io_socket{&ios, fds[1]});

		auto sock1 = static_cast<tlst::io_socket *>(ios.get(fds[0]));
		auto sock2 = static_cast<tlst::io_socket *>(ios.get(fds[1]));

		co_await sock1->send("Hello!", 7);

		char buf[16];
		co_await sock2->recv(buf, 16);

		assert(!strcmp(buf, "Hello!"));
	}(), async::current_queue, ios.get_waiter());
}
