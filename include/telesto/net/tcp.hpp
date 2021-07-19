#pragma once

#include <span>
#include <memory>
#include <optional>
#include <unordered_map>

#include <async/basic.hpp>
#include <async/queue.hpp>
#include <async/result.hpp>
#include <async/algorithm.hpp>
#include <async/recurring-event.hpp>

#include <frg/std_compat.hpp>

#include <telesto/uv_loop.hpp>

#include <uv.h>

namespace tlst {

struct buffer {
	friend void swap(buffer &a, buffer &b) {
		using std::swap;

		swap(a.data_, b.data_);
		swap(a.size_, b.size_);
	}

	buffer() = default;
	buffer(void *data, size_t size)
	: data_{data}, size_{size} { }

	~buffer() {
		operator delete(data_);
	}

	buffer(const buffer &) = delete;
	buffer &operator=(const buffer &) = delete;

	buffer(buffer &&other) {
		swap(*this, other);
	}

	buffer &operator=(buffer &&other) {
		swap(*this, other);
		return *this;
	}

	void *data() const {
		return data_;
	}

	size_t size() const {
		return size_;
	}

private:
	void *data_ = nullptr;
	size_t size_ = 0;
};

struct tcp_connection {
	tcp_connection() = default;

	tcp_connection(uv_loop *loop)
	: loop_{loop} {
		uv_tcp_init(loop_->get(), &h_);

		h_.data = this;
	}

	~tcp_connection() {
		uv_close(reinterpret_cast<uv_handle_t *>(&h_), nullptr);
	}

	tcp_connection(const tcp_connection &) = delete;
	tcp_connection &operator=(const tcp_connection &) = delete;

	tcp_connection(tcp_connection &&) = delete;
	tcp_connection &operator=(tcp_connection &&) = delete;

	uv_tcp_t *get() {
		return &h_;
	}

	uv_stream_t *stream() {
		return reinterpret_cast<uv_stream_t *>(&h_);
	}

	void start_recv() {
		uv_read_start(stream(),
			[] (uv_handle_t *, size_t size, uv_buf_t *buf) {
				buf->base = static_cast<char *>(operator new(size));
				buf->len = size;
			},
			[] (uv_stream_t *stream, ssize_t n, const uv_buf_t *buf) {
				auto conn = static_cast<tcp_connection *>(stream->data);

				if (n == UV_EOF) {
					conn->closed_ = true;
					conn->disconnection_cancellation_.cancel();
					return;
				} else if (n < 0) {
					// TODO: propagate error
					return;
				}

				conn->recv_q_.put(buffer{buf->base, static_cast<size_t>(n)});
			});
	}

private:
	struct send_base {
		friend struct tcp_connection;

		send_base(std::span<const std::byte> data)
		: data_{data} { }

	protected:
		virtual ~send_base() = default;
		virtual void complete() = 0;

	private:
		std::span<const std::byte> data_;
	};

public:
	struct [[nodiscard]] send_sender {
		tcp_connection *conn;
		std::span<const std::byte> data;
	};

	send_sender send(std::span<const std::byte> data) {
		return {this, data};
	}

private:
	template <typename Receiver>
	struct send_operation final : send_base {
		send_operation(send_sender s, Receiver r)
		: send_base{s.data}, conn_{s.conn}, r_{std::move(r)}, op_{} { }

		void start() {
			auto buf = uv_buf_init(
				const_cast<char *>(
					reinterpret_cast<const char *>(
						data_.data())),
				data_.size());

			conn_->send_ops_[&op_] = this;
			uv_write(&op_, conn_->stream(), &buf, 1, [] (uv_write_t *op, int status) {
				auto conn = static_cast<tcp_connection *>(op->handle->data);
				auto send_op = conn->send_ops_[op];
				assert(send_op && "Untracked send op completed with our cb");
				send_op->complete();
			});
		}

	protected:
		void complete() override {
			conn_->send_ops_.erase(&op_);
			async::execution::set_value_noinline(r_);
		}

	private:
		tcp_connection *conn_;
		Receiver r_;
		uv_write_t op_;
	};

public:
	template <typename Receiver>
	friend send_operation<Receiver> connect(send_sender s, Receiver r) {
		return {s, std::move(r)};
	}

	friend async::sender_awaiter<send_sender, void>
	operator co_await(send_sender s) {
		return {s};
	}

	auto recv() {
		return async::transform(recv_q_.async_get(disconnection_cancellation_),
				[] (frg::optional<buffer> buf) -> std::optional<buffer> {
					if (!buf)
						return std::nullopt;
					return std::make_optional(std::move(*buf));
				});
	}

	bool connected() {
		return !closed_;
	}

private:
	uv_tcp_t h_{};
	uv_loop *loop_ = nullptr;

	std::unordered_map<
		uv_write_t *,
		send_base *
	> send_ops_{};

	async::queue<
		buffer,
		frg::stl_allocator
	> recv_q_{};

	bool closed_ = false;

	async::cancellation_event disconnection_cancellation_{};
};

struct tcp_server {
	tcp_server() = default;

	tcp_server(uv_loop *loop)
	: loop_{loop} {
		uv_tcp_init(loop_->get(), &h_);

		h_.data = this;
	}

	~tcp_server() {
		uv_close(reinterpret_cast<uv_handle_t *>(&h_), nullptr);
	}

	tcp_server(const tcp_server &) = delete;
	tcp_server &operator=(const tcp_server &) = delete;

	tcp_server(tcp_server &&) = delete;
	tcp_server &operator=(tcp_server &&) = delete;

	uv_tcp_t *get() {
		return &h_;
	}

	uv_stream_t *stream() {
		return reinterpret_cast<uv_stream_t *>(&h_);
	}

	int bind_v4(const char *ip, int port) {
		sockaddr_in addr;

		if (int r = uv_ip4_addr(ip, port, &addr))
			return r;

		return uv_tcp_bind(&h_, reinterpret_cast<sockaddr *>(&addr), 0);
	}

	int listen(int backlog = 1024) {
		return uv_listen(stream(),
				backlog, [] (uv_stream_t *h, int status) {
			tcp_server *server = static_cast<tcp_server *>(h->data);
			server->accept_backlog_++;
			server->accept_ev_.raise();
		});
	}

	async::result<std::unique_ptr<tcp_connection>> accept() {
		while (true) {
			while (!accept_backlog_) {
				co_await accept_ev_.async_wait();
			}

			auto conn = std::make_unique<tcp_connection>(loop_);

			if (int r = uv_accept(stream(), conn->stream())) {
				co_return nullptr; // TODO: propagate error
			}

			conn->start_recv();

			accept_backlog_--;

			co_return conn;
		}
	}

private:
	uv_tcp_t h_ {};
	uv_loop *loop_ = nullptr;

	int accept_backlog_ = 0;
	async::recurring_event accept_ev_;
};

} // namespace tlst
