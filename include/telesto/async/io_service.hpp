#pragma once

#include <telesto/platform/default_poll_engine.hpp>
#include <telesto/io/poll.hpp>
#include <unordered_map>
#include <memory>

namespace tlst {

struct file;

struct io_service {
	io_service()
	: engine_{std::make_unique<default_poll_engine>()}, monitored_objects_{} { }

	io_service(poll_engine *e)
	: engine_{e}, monitored_objects_{} { }

	virtual ~io_service() = default;

	// TODO(qookie): this takes ownership of the object, find a more descriptive way of doing it
	void add(pollable *p) {
		int fd = p->fd();
		monitored_objects_.emplace(fd, p);
		engine_->add(fd, true);
	}

	void remove(int fd) {
		engine_->remove(fd);
		monitored_objects_.erase(fd);
	}

	pollable *get(int fd) {
		return monitored_objects_[fd].get();
	}

	void wait() {
		auto evs = engine_->wait(10);

		// TODO(qookie): maybe this logic should be moved into poll_engine?
		for (auto ev : evs) {
			if (ev.kind & event_kind::readable)
				monitored_objects_[ev.fd]->on_readable();
			if (ev.kind & event_kind::writable)
				monitored_objects_[ev.fd]->on_writable();
			if (ev.kind & event_kind::hung_up)
				monitored_objects_[ev.fd]->on_hang_up();
		}
	}

	struct io_waiter {
		void wait() {
			parent_->wait();
		}

		io_service *parent_;
	};

	io_waiter get_waiter() {
		return {this};
	}

private:
	std::unique_ptr<poll_engine> engine_;
	std::unordered_map<int, std::unique_ptr<pollable>> monitored_objects_;
};

} // namespace tlst
