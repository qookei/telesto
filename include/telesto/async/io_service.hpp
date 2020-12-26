#pragma once

namespace tlst {

struct file;

struct io_waiter {
	virtual ~io_waiter() = default;

	virtual void wait() = 0;
};

struct io_service {
	virtual ~io_service() = default;

	virtual void add(file &p) = 0;
	virtual void remove(file &p) = 0;
};

} // namespace tlst
