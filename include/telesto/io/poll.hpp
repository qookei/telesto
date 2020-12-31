#pragma once

#include <vector>

namespace tlst {

namespace event_kind {
	static constexpr int readable = 1;
	static constexpr int writable = 2;
	static constexpr int hung_up = 4;
};

struct poll_event {
	int fd;
	int kind;
};

struct poll_engine {
	virtual ~poll_engine() = default;

	virtual void add(int fd, bool edge_triggered) = 0;
	virtual void remove(int fd) = 0;
	virtual std::vector<poll_event> wait(int timeout) = 0;
};

struct pollable {
	virtual ~pollable() = default;

	virtual void on_readable() = 0;
	virtual void on_writable() = 0;
	virtual void on_hang_up() = 0;
	virtual int fd() = 0;
};

} // namespace tlst
