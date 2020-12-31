#pragma once

#if defined(__linux__) || defined(__managarm__)
#	include <telesto/platform/epoll/epoll_poll_engine.hpp>
#else
// TODO(qookie): add kqueue(2) support
#	include <telesto/platform/poll/poll_poll_engine.hpp>
#endif

namespace tlst {

#if defined(__linux__) || defined(__managarm__)
	using default_poll_engine = platform::epoll_poll_engine;
#else
// TODO(qookie): add kqueue(2) support
	using default_poll_engine = platform::poll_poll_engine;
#endif

} // namespace tlst
