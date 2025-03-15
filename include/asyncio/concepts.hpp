#pragma once
#include <coroutine>
#include <utility>

#include "asyncio_ns.hpp"


ASYNCIO_NS_BEGIN(concepts)

template<typename C>
concept Cancelable = requires(C canceler) {
    { canceler.cancel() } -> std::same_as<void>;
    { canceler.canceled() } -> std::same_as<bool>;
};


template<typename A>
concept Awaitable = requires(A awaiter) {
    { awaiter.await_ready() } -> std::same_as<bool>;
    { awaiter.await_resume() };
    requires requires(std::coroutine_handle<> handle) {
        { awaiter.await_suspend(handle) } -> std::same_as<void>;
    } || requires(std::coroutine_handle<> handle) {
        { awaiter.await_suspend(handle) } -> std::same_as<bool>;
    };
};


template<typename T>
concept Task = Cancelable<T> && Awaitable<T> && requires(T task) {
    typename T::Callback;
    typename T::result_type;
    typename T::promise_type;
    requires std::move_constructible<T>;
    requires !std::default_initializable<T>;
    requires !std::copy_constructible<T>;
    { task.done() } -> std::same_as<bool>;
    { task.valid() } -> std::same_as<bool>;
    { task.result() } -> std::same_as<typename T::result_type>;
    requires requires(typename T::Callback cb) {
        { task.add_done_callback(std::move(cb)) } -> std::same_as<void>;
    };
};


template<typename P>
concept Promise = requires(P promise) {
    { promise.get_return_object() } -> Task;
    { promise.initial_suspend() } -> Awaitable;
    { promise.final_suspend() } noexcept -> Awaitable;
    { promise.unhandled_exception() } -> std::same_as<void>;
};

ASYNCIO_NS_END
