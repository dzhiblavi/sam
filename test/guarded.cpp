// Copyright (c) 2022 Klemens D. Morgenstern, Ricahrd Hodges
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

#include <boost/asem/mt.hpp>
#include <boost/asem/st.hpp>
#include <boost/asem/guarded.hpp>
#include <chrono>
#include <random>
#include <vector>

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

static int concurrent = 0;

struct impl
{
    int id;
    std::shared_ptr<asio::steady_timer> tim;
    impl(int id, asio::any_io_executor exec) : id(id), tim{std::make_shared<asio::steady_timer>(exec)}
    {
        assert(exec);
    }

    template<typename Self>
    void operator()(Self && self)
    {
        BOOST_CHECK_LE(concurrent, 3);
        concurrent ++;
        printf("Entered %d\n", id);
        tim->expires_after(std::chrono::milliseconds{10});
        tim->async_wait(std::move(self));
    }
    template<typename Self>
    void operator()(Self && self, error_code ec)
    {
        BOOST_CHECK(!ec);
        printf("Exited %d %d\n", id, ec.value());
        concurrent --;
        self.complete(ec);
    }
};

template<typename T, typename CompletionToken>
auto async_impl(T & se, int i, CompletionToken && completion_token)
{
    return BOOST_ASEM_ASIO_NAMESPACE::async_compose<CompletionToken, void(error_code)>(
            impl(i, se.get_executor()), completion_token, se.get_executor());
}

template<typename T>
void test_sync(T & se2, std::vector<int> & order)
{
    auto op =
            [&](auto && token)
            {
                static int i = 0;
                return async_impl(se2, i ++, std::move(token));
            };


    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
    guarded(se2, op,  asio::detached);
}


BOOST_AUTO_TEST_CASE_TEMPLATE(guarded_semaphore_test, T, models)
{
    context<T> ctx;
    typename T::semaphore se{ctx.get_executor(), 3};
    std::vector<int> order;
    test_sync<basic_semaphore<T>>(se, order);
    run_impl(ctx);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(guarded_mutex_test, T, models)
{
    context<T> ctx;
    std::vector<int> order;
    typename T::mutex mtx{ctx.get_executor()};
    test_sync<basic_mutex<T>>(mtx, order);
    run_impl(ctx);
}