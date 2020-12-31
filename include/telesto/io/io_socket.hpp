#pragma once

#include <telesto/async/io_service.hpp>
#include <telesto/io/poll.hpp>
#include <async/result.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <cassert>
#include <fcntl.h>
#include <queue>

namespace tlst {

struct io_socket final : pollable {
	io_socket(io_service *owner, int fd)
	: owner_{owner}, fd_{fd} {
		int flags = fcntl(fd_, F_GETFL, 0);
		assert(flags > -1);
		flags |= O_NONBLOCK;
		fcntl(fd_, F_SETFL, flags);
	}

	~io_socket() {
		if (owner_)
			owner_->remove(*this);

		close(fd_);
	}

	io_socket(const io_socket &) = delete;
	io_socket(io_socket &&) = default;

	io_socket &operator=(const io_socket &) = delete;
	io_socket &operator=(io_socket &&) = default;

	io_service *get_io_service() const {
		return owner_;
	}

	int fd() override {
		return fd_;
	}

	operator bool() const {
		return fd_ >= 0;
	}

private:
	struct send_base {
		friend struct io_socket;

		send_base(const void *buffer, size_t size)
		: buffer_{buffer}, size_{size} { }

	protected:
		virtual ~send_base() = default;

		virtual void complete() = 0;

	private:
		const void *buffer_;
		size_t size_;
	};

	struct recv_base {
		friend struct io_socket;

		recv_base(void *buffer, size_t size)
		: buffer_{buffer}, size_{size} { }

	protected:
		virtual ~recv_base() = default;

		virtual void complete() = 0;

	private:
		void *buffer_;
		size_t size_;
	};

public:
	struct [[nodiscard]] send_sender {
		io_socket *sock;
		const void *buffer;
		size_t size;
	};

	send_sender send(const void *buf, size_t size) {
		return {this, buf, size};
	}

	template <typename Receiver>
	struct send_operation final : send_base {
		friend struct io_socket;

		send_operation(send_sender s, Receiver r)
		: send_base{s.buffer, s.size}, sock_{s.sock}, r_{std::move(r)} { }

		void start() {
			if (sock_->send_q_.empty()) {
				if (sock_->progress_send_(this)) {
					complete();
					return;
				}
			}

			sock_->send_q_.push(this);
		}

	protected:
		void complete() override {
			r_.set_value();
		}

	private:
		io_socket *sock_;
		Receiver r_;
	};

	template <typename Receiver>
	friend send_operation<Receiver> connect(send_sender s, Receiver r) {
		return {s, std::move(r)};
	}

	friend async::sender_awaiter<send_sender, void>
	operator co_await(send_sender s) {
		return {s};
	};

	struct [[nodiscard]] recv_sender {
		io_socket *sock;
		void *buffer;
		size_t size;
	};

	recv_sender recv(void *buf, size_t size) {
		return {this, buf, size};
	}

	template <typename Receiver>
	struct recv_operation final : recv_base {
		friend struct io_socket;

		recv_operation(recv_sender s, Receiver r)
		: recv_base{s.buffer, s.size}, sock_{s.sock}, r_{std::move(r)} { }

		void start() {
			if (sock_->recv_q_.empty()) {
				if (sock_->progress_recv_(this)) {
					complete();
					return;
				}
			}

			sock_->recv_q_.push(this);
		}

	protected:
		void complete() override {
			r_.set_value();
		}

	private:
		io_socket *sock_;
		Receiver r_;
	};

	template<typename Receiver>
	friend recv_operation<Receiver> connect(recv_sender s, Receiver r) {
		return {s, std::move(r)};
	}

	friend async::sender_awaiter<recv_sender, void>
	operator co_await(recv_sender s) {
		return {s};
	};

	void on_readable() override {
		while (!recv_q_.empty()) {
			auto op = recv_q_.front();

			if(!progress_recv_(op))
				break;

			recv_q_.pop();

			op->complete();
		}
	}

	void on_writable() override {
		while (!send_q_.empty()) {
			auto op = send_q_.front();

			if(!progress_send_(op))
				break;

			send_q_.pop();

			op->complete();
		}
	}

	void on_hang_up() override {
		std::cout << "TODO: Remote disconnected" << std::endl;
	}

private:
	bool progress_send_(send_base *op) {
		iovec iov = {
			.iov_base = const_cast<void *>(op->buffer_),
			.iov_len = op->size_,
		};

		msghdr hdr = {
			.msg_iov = &iov,
			.msg_iovlen = 1,
		};

		ssize_t chunk = sendmsg(fd(), &hdr, MSG_DONTWAIT);

		if (chunk < 0) {
			assert(errno == EAGAIN || errno == EWOULDBLOCK);
			return false;
		}

		assert(chunk == static_cast<ssize_t>(op->size_));

		return true;
	}

	bool progress_recv_(recv_base *op) {
		iovec iov = {
			.iov_base = op->buffer_,
			.iov_len = op->size_,
		};

		msghdr hdr = {
			.msg_iov = &iov,
			.msg_iovlen = 1,
		};

		ssize_t chunk = recvmsg(fd(), &hdr, MSG_DONTWAIT);

		if (chunk < 0) {
			assert(errno == EAGAIN || errno == EWOULDBLOCK);
			return false;
		}

		assert(!(hdr.msg_flags & MSG_TRUNC));

		return true;
	}

private:
	io_service *owner_;
	int fd_;

	std::queue<send_base *> send_q_;
	std::queue<recv_base *> recv_q_;
};

} // namespace tlst
