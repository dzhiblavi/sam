// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

#include <boost/asem/lock_guard.hpp>
#include <boost/asem/mt.hpp>
#include <boost/asem/st.hpp>
#include <chrono>
#include <random>

#if !defined(BOOST_ASEM_STANDALONE)
namespace asio = BOOST_ASEM_ASIO_NAMESPACE;
#include <boost/asio.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/yield.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#else
#include <asio.hpp>
#include <asio/compose.hpp>
#include <asio/yield.hpp>
#include <asio/experimental/parallel_group.hpp>
#endif

using namespace BOOST_ASEM_NAMESPACE;
using namespace BOOST_ASEM_ASIO_NAMESPACE;
using namespace BOOST_ASEM_ASIO_NAMESPACE::experimental;

using models = std::tuple<st, mt>;
template<typename T>
using context = typename std::conditional<
        std::is_same<st, T>::value,
        io_context,
        thread_pool
    >::type;

inline void run_impl(io_context & ctx)
{
    ctx.run();
}

inline void run_impl(thread_pool & ctx)
{
    ctx.join();
}

/*
asio::awaitable<void> main_impl()
{
    using executor = asio::use_awaitable_t<>::executor_with_default<asio::any_io_executor>;
    asioex::basic_mutex<executor> mtx { co_await asio::this_coro::executor };

    std::vector< int > seq;

    auto f = [](std::vector< int > &v, asioex::basic_mutex<executor> &mtx, int i) -> asio::awaitable< void >
    {
        auto l = co_await asioex::async_guard(mtx);
        v.push_back(i);
        asio::steady_timer tim { co_await asio::this_coro::executor, std::chrono::milliseconds(10) };
        co_await tim.async_wait(asio::use_awaitable);
        v.push_back(i + 1);
    };

    using asio::experimental::awaitable_operators::operator&&;

    co_await (f(seq, mtx, 0) && f(seq, mtx, 3) && f(seq, mtx, 6) && f(seq, mtx, 9));

    assert(seq.size() == 8);
    assert(seq[0] + 1 == seq[1]);
    assert(seq[2] + 1 == seq[3]);
    assert(seq[4] + 1 == seq[5]);
    assert(seq[6] + 1 == seq[7]);
}*/

template<typename T>
struct basic_main_impl
{
    typename T::mutex mtx;
    std::vector< int > seq;

    basic_main_impl(BOOST_ASEM_ASIO_NAMESPACE::any_io_executor exec) : mtx{exec} {}

};

template<typename T>
struct basic_main : BOOST_ASEM_ASIO_NAMESPACE::coroutine
{
    struct step_impl
    {
        std::vector< int > &v;
        typename T::mutex &mtx;
        int i;

        std::unique_ptr<asio::steady_timer> tim;
        template<typename Self>
        void operator()(Self & self)
        {
            async_lock(mtx, std::move(self));
        }

        template<typename Self>
        void operator()(Self & self, error_code ec, lock_guard<typename T::mutex> l)
        {
            v.push_back(i);
            tim = std::make_unique<asio::steady_timer>(mtx.get_executor(), std::chrono::milliseconds(10));
            tim->async_wait(std::move(self));
            v.push_back(i + 1);
        };

        template<typename Self>
        void operator()(Self & self, error_code ec)
        {
            self.complete(ec);
        }
    };

    basic_main(BOOST_ASEM_ASIO_NAMESPACE::any_io_executor exec) : impl_(std::make_unique<basic_main_impl<T>>(exec)) {}

    std::unique_ptr<basic_main_impl<T>> impl_;

    static auto f(std::vector< int > &v, typename T::mutex &mtx, int i)
        -> asio::deferred_async_operation<void (error_code),
                asio::detail::initiate_composed_op<void (error_code),
                                                   void (asio::any_io_executor)>, step_impl>
    {
        return async_compose<const asio::deferred_t &, void(error_code)>(
                step_impl{v, mtx, i},
                asio::deferred, mtx);
    }


    void operator()(error_code = {})
    {
        reenter (this)
        {
            yield make_parallel_group(
                  f(impl_->seq, impl_->mtx, 0),
                  f(impl_->seq, impl_->mtx, 3),
                  f(impl_->seq, impl_->mtx, 6),
                  f(impl_->seq, impl_->mtx, 9))
                    .async_wait(wait_for_all(), std::move(*this));

        }
    }

    void operator()(std::array<std::size_t, 4> order,
                    error_code ec1,  error_code ec2,
                    error_code ec3,  error_code ec4)
    {
        BOOST_CHECK(!ec1);
        BOOST_CHECK(!ec2);
        BOOST_CHECK(!ec3);
        BOOST_CHECK(!ec4);
        BOOST_CHECK(impl_->seq.size() == 8);
        BOOST_CHECK_EQUAL(impl_->seq[0] + 1, impl_->seq[1]);
        BOOST_CHECK_EQUAL(impl_->seq[2] + 1, impl_->seq[3]);
        BOOST_CHECK_EQUAL(impl_->seq[4] + 1, impl_->seq[5]);
        BOOST_CHECK_EQUAL(impl_->seq[6] + 1, impl_->seq[7]);
    }

};

BOOST_AUTO_TEST_CASE_TEMPLATE(random_mtx, T, models)
{
    context<T> ctx;
    BOOST_ASEM_ASIO_NAMESPACE::post(ctx, basic_main<T>{ctx.get_executor()});
    run_impl(ctx);
}