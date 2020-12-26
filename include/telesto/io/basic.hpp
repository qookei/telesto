#pragma once

#include <telesto/async/io_service.hpp>

namespace tlst {

struct file {
	file(io_service *service)
	: service_{service} { }

	virtual ~file() = default;

	file(const file &) = delete;
	file(file &&) = default;

	file &operator=(const file &) = delete;
	file &operator=(file &&) = default;

	io_service *get_io_service() const {
		return service_;
	}

	virtual void on_readable() = 0;
	virtual void on_writable() = 0;
	virtual void on_hang_up() = 0;

private:
	io_service *service_;
};

} // namespace tlst
