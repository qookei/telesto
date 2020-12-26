#include <telesto/platform/linux/io_socket.hpp>
#include <telesto/platform/linux/io_epoll.hpp>
#include <async/result.hpp>
#include <sys/socket.h>

int main() {
	async::run_queue rq;
	async::queue_scope qs{&rq};
	tlst::epoll_io_service ios;

	async::run([&ios] () -> async::result<void> {
		int fds[2];
		assert(!socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds));

		tlst::io_socket sock1{&ios, fds[0]};
		tlst::io_socket sock2{&ios, fds[1]};

		co_await sock1.send("Hello!", 7);

		char buf[16];
		co_await sock2.recv(buf, 16);

		assert(!strcmp(buf, "Hello!"));
	}(), async::current_queue, ios.get_waiter());
}
