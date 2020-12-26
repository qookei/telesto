#pragma once

#include <telesto/io/basic.hpp>
#include <unistd.h>

namespace tlst {

// TODO(qookie): this is not entirely linux specific
struct fd_file : file {
	fd_file(io_service *service, int fd)
	: file{service}, fd_{fd} { }

	virtual ~fd_file() { 
		close(fd_);
	}

	int fd() const {
		return fd_;
	}

	operator bool() const {
		return fd_ >= 0;
	}

private:
	int fd_;
};

} // namespace tlst
