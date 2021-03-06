#pragma once
#include <variant>
#include <optional>
#include <coroutine>
#include <vector>
#include "ref_count_ptr.h"
#include "better_conditional.h"
#include <type_traits>

namespace cerwy {

class something_happened final :public std::exception {
	using std::exception::exception;
};

struct no_state {};

template<typename T, bool does_move>
struct shared_state_base_non_void :ref_counted {
	using value_type = T;

	bool await_ready() {
		return !std::holds_alternative<no_state>(m_data);
	}

	T value() {
		return std::visit([&](auto& a)->T {
			using type = std::decay_t<decltype(a)>;
			if constexpr (std::is_same_v<type, T>) {
				if constexpr (does_move) {
					return std::move(a);
				} else {
					return a;
				}
			} else if constexpr (std::is_same_v<type, no_state>) {
				throw something_happened{};
			} else {
				std::rethrow_exception(a);
			}
		}, m_data);
	}

protected:
	void set_value__(T a) {
		m_data.template emplace<T>(std::move(a));
	}

	void set_exception__(const std::exception_ptr e) {
		m_data = e;
	}

private:
	std::variant<T, std::exception_ptr, no_state> m_data = no_state{};
};

struct shared_state_base_void :ref_counted {
	using value_type = void;

	bool await_ready() const noexcept {
		return m_is_done.load();
	}

	void value() {
		if (m_watland) {
			std::rethrow_exception(*m_watland);
		}
	}

protected:
	void set_value__() {
		m_is_done.store(true);
	}

	void set_exception__(const std::exception_ptr& e) {
		m_watland = e;
	}

private:
	std::atomic<bool> m_is_done = false;
	std::optional<std::exception_ptr> m_watland = std::nullopt;
};

template<typename T, bool does_move>
using shared_state_base = better_conditional_t<std::is_void_v<T>, shared_state_base_void, shared_state_base_non_void<T, does_move>>;

template<typename T>
struct shared_shared_state_base :shared_state_base<T, false> {
	
	void on_await_suspend(const std::coroutine_handle<> h) {		
		m_handles.push_back(h);
	}

	void set_exception(std::exception_ptr e) {
		this->set_exception__(e);
	}

protected:
	std::vector<std::coroutine_handle<>> m_handles;
};

template<typename T>
struct shared_shared_state :shared_shared_state_base<T> {
	void set_value(T a) {
		this->set_value__(std::move(a));
		for (auto& i : this->m_handles) {
			i.resume();
		}
	}
	/*
	template<typename executor_t>
	void set_value(T a,executor_t&& exec){
		this->set_value__(std::move(a));
		for(auto& i:m_handles){
			exec.execute([=,i](){i.resume();});
		}
	}

	*/

};

template<>
struct shared_shared_state<void> :shared_shared_state_base<void> {
	void set_value() {
		this->set_value__();
		for (auto& i : this->m_handles) {
			i.resume();
		}
	}
	/*
	template<typename executor_t>
	void set_value(executor_t&& exec){
		this->set_value__();
		for(auto& i:this->m_handles){
			exec.execute([=,i](){i.resume();});
		}
	}

	*/

};

template<typename T>
struct non_shared_shared_state_base :shared_state_base<T, true> {
	void on_await_suspend(std::coroutine_handle<> h) {
		//[[assert:!m_awaiter]];
		m_awaiter = h;
	}

	void set_exception(std::exception_ptr e) {
		this->set_exception__(e);
	}

protected:
	std::coroutine_handle<> m_awaiter;
};

template<typename T>
struct non_shared_shared_state :non_shared_shared_state_base<T> {
	void set_value(T a) {
		this->set_value__(std::move(a));
		if (this->m_awaiter) {
			this->m_awaiter.resume();
		}
	}
};

template<>
struct non_shared_shared_state<void> :non_shared_shared_state_base<void> {
	void set_value() {
		this->set_value__();
		if (this->m_awaiter) {
			this->m_awaiter.resume();
		}
	}
	/*
	template<typename executor_t>
	void set_value(executor_t&& exec) {
		this->set_value__();
		if (this->m_awaiter)
			this->m_awaiter.resume();
	}*/
};

template<typename shared_state_t>
struct managed_shared_state {
	shared_state_t& shared_state() {
		return *m_state;
	}

protected:
	managed_shared_state() = default;

	explicit managed_shared_state(ref_count_ptr<shared_state_t> state) :
		m_state(std::move(state)) {}

	ref_count_ptr<shared_state_t> m_state;
};

template<typename shared_state_t>
struct templated_task :managed_shared_state<shared_state_t> {
	using shared_state_type = shared_state_t;
	using value_type = typename shared_state_t::value_type;

	templated_task() = default;

	explicit templated_task(ref_count_ptr<shared_state_t> a) :
		managed_shared_state<shared_state_t>(std::move(a)) {};

	bool await_ready() {
		return this->m_state->await_ready();
	}

	void await_suspend(std::coroutine_handle<> h) {
		this->m_state->on_await_suspend(h);
	}

	decltype(auto) await_resume() {
		return this->m_state->value();
	}

	struct promise_type;
};

struct empty_promise_t {};

static constexpr inline empty_promise_t empty_promise;

template<typename shared_state_t>
struct templated_promise_base :managed_shared_state<shared_state_t> {
	using value_type = typename shared_state_t::value_type;

	explicit templated_promise_base() :
		managed_shared_state<shared_state_t>(make_ref_count_ptr<shared_state_t>()) { }

	explicit templated_promise_base(empty_promise_t) noexcept { }

	explicit templated_promise_base(ref_count_ptr<shared_state_t> s) :
		managed_shared_state<shared_state_t>(std::move(s)) { }

	void set_shared_state(ref_count_ptr<shared_state_t> s) {
		this->m_state = std::move(s);
	}

	auto get_task() {
		return templated_task<shared_state_t>(this->m_state);
	};

	void set_exception(std::exception_ptr e) {
		this->m_state->set_exception(e);
	}
};

template<typename shared_state_t, typename = void>
struct templated_promise :templated_promise_base<shared_state_t> {
	using value_type = typename shared_state_t::value_type;
	using templated_promise_base<shared_state_t>::templated_promise_base;

	void set_value(value_type v) {
		this->m_state->set_value(std::move(v));
	}

	/*
	template<typename executor_t = immediate_executor>
	void set_value(value_type v,executor_t&& e = {}) {
		this->m_state->set_value(std::move(v),std::forward<executor_t>(e));
	}
	*/
};

template<typename shared_state_t>
struct templated_promise<shared_state_t, std::enable_if_t<std::is_void_v<typename shared_state_t::value_type>>> :templated_promise_base<shared_state_t> {
	using value_type = typename shared_state_t::value_type;
	using templated_promise_base<shared_state_t>::templated_promise_base;

	void set_value() {
		this->m_state->set_value();
	}

	/*
	template<typename executor_t>
	void set_value(executor_t&& e) {
		this->m_state->set_value(std::forward<executor_t>(e));
	}
	*/
};

template<typename shared_state_t>
struct promise_base_common {
	using task_t = templated_task<shared_state_t>;
	using promise_t = templated_promise<shared_state_t>;

	task_t get_return_object() {
		return m_promise.get_task();
	}

	void unhandled_exception() {
		m_promise.set_exception(std::current_exception());
	}

protected:
	promise_t m_promise;
};

template<typename T, typename shared_state_t>
struct promise_base :promise_base_common<shared_state_t> {
	void return_value(T a) {
		this->m_promise.set_value(std::move(a));
	}
};

template<typename shared_state_t>
struct promise_base<void, shared_state_t> :promise_base_common<shared_state_t> {
	void return_void() {
		this->m_promise.set_value();
	}
};

template<typename shared_state_t>
struct templated_task<shared_state_t>::promise_type :promise_base<value_type, shared_state_t> {
	static std::suspend_never initial_suspend() {
		return {};
	}

	static std::suspend_never final_suspend() noexcept{
		return {};
	}
};

template<typename T = void>
using eager_task = templated_task<non_shared_shared_state<T>>;

template<typename T = void>
using promise = templated_promise<non_shared_shared_state<T>>;

template<typename T = void>
using shared_task = templated_task<shared_shared_state<T>>;

template<typename T = void>
using shared_promise = templated_promise<shared_shared_state<T>>;

template<typename T>
eager_task<T> make_ready_task(T a) {
	promise<T> p;
	auto r = p.get_task();
	p.set_value(std::move(a));
	return r;
}

inline eager_task<void> make_ready_void_task() {
	promise<void> p;
	auto r = p.get_task();
	p.set_value();
	return r;
}

const static eager_task<void> completed_action = make_ready_void_task();


template<typename T>
using awaitable_result_t = decltype(std::declval<T>().await_result());

template<typename Awaitable>
auto async_invoke(Awaitable&& awaitable)->eager_task<awaitable_result_t<Awaitable>> {
	co_return co_await awaitable;
}

//is this even usefull?
template<typename callable>
eager_task<std::invoke_result_t<callable>> make_async(callable&& fn) {
	co_return std::invoke(fn);
}

//can't check if finishes, I can save 1 whole allocation!!!!111!!!!
struct unawaitable_action {
	struct promise_type {
		// ReSharper disable CppMemberFunctionMayBeStatic
		unawaitable_action  get_return_object()const noexcept {
			return {};
		}

		void unhandled_exception() const noexcept {
			//do nothing
		}
			
		void return_void()const noexcept {
			
		}
		
		std::suspend_never initial_suspend() const noexcept {
			return {};
		}

		std::suspend_never final_suspend() const noexcept{
			return {};
		}
		// ReSharper restore CppMemberFunctionMayBeStatic
	};

};

};
