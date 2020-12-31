#pragma once

#include <telesto/io/poll.hpp>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <poll.h>
#include <vector>

namespace tlst::platform {

struct poll_poll_engine final : poll_engine {
	void add(int fd, bool edge_triggered) override {
		pollfd pf;
		pf.fd = fd;
		pf.events = POLLIN | POLLOUT;

		if (!edge_triggered)
			std::cerr << "poll_poll_engine: poll behaves as if all fds are edge triggered"
				<< " (adding fd " << fd << " as level triggered)" << std::endl;

		fds_.push_back(pf);
	}

	void remove(int fd) override {
		fds_.erase(std::remove_if(fds_.begin(), fds_.end(),
					[&] (auto pf) { return pf.fd == fd; } ),
				fds_.end());
	}

	std::vector<poll_event> wait(int timeout) override {
		int ret = poll(fds_.data(), fds_.size(), timeout);

		if (ret < 0) {
			assert(errno == EINTR);
			return {};
		}

		std::vector<poll_event> out_evs;
		for (auto &pf : fds_) {
			if (!pf.revents)
				continue;

			poll_event out_ev{};

			out_ev.fd = pf.fd;

			if (pf.revents & POLLIN)
				out_ev.kind |= event_kind::readable;
			if (pf.revents & POLLOUT)
				out_ev.kind |= event_kind::writable;
			if (pf.revents & POLLHUP)
				out_ev.kind |= event_kind::hung_up;

			pf.revents = 0;

			out_evs.push_back(out_ev);
		}

		return out_evs;
	}

private:
	std::vector<pollfd> fds_;
};

} // namespace tlst::platform
