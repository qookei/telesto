#pragma once

#include <utility>
#include <uv.h>

namespace tlst {

struct uv_loop {
	friend void swap(uv_loop &a, uv_loop &b) {
		using std::swap;
		swap(a.loop_, b.loop_);
	}

	uv_loop(nullptr_t) { }

	uv_loop() {
		uv_loop_init(&loop_);
	}

	~uv_loop() {
		uv_loop_close(&loop_);
	}

	uv_loop(const uv_loop &) = delete;
	uv_loop &operator=(const uv_loop &) = delete;

	uv_loop(uv_loop &&other)
	: uv_loop{nullptr} {
		swap(*this, other);
	}

	uv_loop &operator=(uv_loop &&other) {
		swap(*this, other);
		return *this;
	}

	uv_loop_t *get() {
		return &loop_;
	}

private:
	uv_loop_t loop_{};
};

struct uv_loop_service {
	uv_loop_service(uv_loop *loop)
	: loop_{loop} { }

	void wait() {
		uv_run(loop_->get(), UV_RUN_ONCE);
	}

private:
	uv_loop *loop_;
};

} // namespace tlst
