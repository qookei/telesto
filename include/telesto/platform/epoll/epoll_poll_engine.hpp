#pragma once

#include <telesto/io/poll.hpp>
#include <sys/epoll.h>
#include <algorithm>
#include <iterator>
#include <unistd.h>
#include <cassert>

namespace tlst::platform {

struct epoll_poll_engine final : poll_engine  {
	epoll_poll_engine()
	: fd_{epoll_create1(EPOLL_CLOEXEC)} { }

	~epoll_poll_engine() {
		close(fd_);
	}

	void add(int fd, bool edge_triggered) override {
		epoll_event ev;
		ev.data.fd = fd;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP;

		if (edge_triggered)
			ev.events |= EPOLLET;

		epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev);
	}

	void remove(int fd) override {
		epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
	}

	std::vector<poll_event> wait(int timeout) override {
		epoll_event ep_evs[32];
		int ret = epoll_wait(fd_, ep_evs, 32, timeout);

		if (ret < 0) {
			assert(errno == EINTR);
			return {};
		}

		std::vector<poll_event> out_evs;

		std::transform(ep_evs, ep_evs + ret, std::back_inserter(out_evs),
			[] (epoll_event ep_ev) -> poll_event {
				poll_event out_ev{};

				out_ev.fd = ep_ev.data.fd;

				if (ep_ev.events & EPOLLIN)
					out_ev.kind |= event_kind::readable;
				if (ep_ev.events & EPOLLOUT)
					out_ev.kind |= event_kind::writable;
				if (ep_ev.events & EPOLLHUP)
					out_ev.kind |= event_kind::hung_up;

				return out_ev;
			}
		);

		return out_evs;
	}

private:
	int fd_;
};

} // namespace tlst::platform
