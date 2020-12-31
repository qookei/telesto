#pragma once

#if defined(__linux__) || defined(__managarm__)
#	include <telesto/platform/epoll/epoll_poll_engine.hpp>
#else
// TODO(qookie): add poll(3p) and kqueue(2) support
#	error Unsupported platform
#endif

namespace tlst {

#if defined(__linux__) || defined(__managarm__)
	using default_poll_engine = platform::epoll_poll_engine;
#else
// TODO(qookie): add poll(3p) and kqueue(2) support
#	error Unsupported platform
#endif

} // namespace tlst
