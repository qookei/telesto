#pragma once

#include <telesto/platform/linux/file.hpp>
#include <telesto/async/io_service.hpp>
#include <frg/expected.hpp>
#include <system_error>
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cassert>
#include <span>

namespace tlst {

struct epoll_file : fd_file {
	epoll_file()
	: fd_file{nullptr, epoll_create1(EPOLL_CLOEXEC)} { }

	~epoll_file() = default;

	frg::expected<std::errc, void> add_fd(fd_file &file) {
		epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLET;
		ev.data.ptr = &file;

		if (epoll_ctl(fd(), EPOLL_CTL_ADD, file.fd(), &ev))
			return static_cast<std::errc>(errno);

		return {};
	}

	frg::expected<std::errc, void> remove_fd(fd_file &file) {
		if (epoll_ctl(fd(), EPOLL_CTL_DEL, file.fd(), nullptr))
			return static_cast<std::errc>(errno);
		return {};
	}

	frg::expected<std::errc, int> wait(std::span<epoll_event> evs, int timeout) {
		int ret = epoll_wait(fd(), evs.data(), evs.size(), timeout);

		if (ret >= 0)
			return ret;
		else
			return static_cast<std::errc>(errno);
	}

	void on_readable() override { };
	void on_writable() override { };
	void on_hang_up() override { };
};

struct epoll_io_service : io_service {
	epoll_io_service()
	: epoll_file_{} { }

	void add(file &p) override {
		fd_file &f = static_cast<fd_file &>(p);

		auto r = epoll_file_.add_fd(f);
		if (!r) {
			std::cerr << "epoll_io_service::add failed: " << std::make_error_code(r.error()).message() << std::endl;
		}
	}

	void remove(file &p) override {
		fd_file &f = static_cast<fd_file &>(p);

		auto r = epoll_file_.add_fd(f);
		if (!r) {
			std::cerr << "epoll_io_service::remove failed: " << std::make_error_code(r.error()).message() << std::endl;
		}
	}

	struct epoll_waiter : io_waiter {
		epoll_waiter(epoll_file *f)
		: file{f} { }

		void wait() {
			while (true) {
				epoll_event evs[16];
				auto r = file->wait(evs, 100);

				if (!r) {
					if (r.error() == std::errc::interrupted)
						return;

					std::cerr << "epoll_io_service::wait failed: " << std::make_error_code(r.error()).message() << std::endl;
					abort();
				}

				int n = r.value();

				if (n)
					return;

				for (int i = 0; i < n; i++) {
					const epoll_event &ev = evs[i];

					auto f = static_cast<fd_file *>(ev.data.ptr);
					if(ev.events & EPOLLIN)
						f->on_readable();

					if(ev.events & EPOLLOUT)
						f->on_writable();

					if(ev.events & EPOLLHUP)
						f->on_hang_up();
				}
			}
		}

		epoll_file *file;
	};

	epoll_waiter get_waiter() {
		return {&epoll_file_};
	}

private:
	epoll_file epoll_file_;
};

} // namespace tlst
