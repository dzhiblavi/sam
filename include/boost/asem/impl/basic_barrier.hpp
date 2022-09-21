//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_IMPL_BASIC_BARRIER_HPP
#define BOOST_ASEM_IMPL_BASIC_BARRIER_HPP

#include <boost/asem/basic_barrier.hpp>
#include <boost/asem/detail/basic_op_model.hpp>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/deferred.hpp>
#include <asio/post.hpp>
#else
#include <boost/asio/deferred.hpp>
#include <boost/asio/post.hpp>
#endif


BOOST_ASEM_BEGIN_NAMESPACE

template < class  Implementation, class Executor >
struct basic_barrier<Implementation ,Executor>::async_arrive_op
{
    basic_barrier<Implementation, Executor> * self;

    template< class Handler >
    void operator()(Handler &&handler)
    {
        auto e = get_associated_executor(handler, self->get_executor());

        if (self->impl_.try_arrive())
            return BOOST_ASEM_ASIO_NAMESPACE::post(
                    std::move(e),
                    BOOST_ASEM_ASIO_NAMESPACE::append(
                            std::forward< Handler >(handler), error_code()));

        auto l = self->impl_.lock();
        ignore_unused(l);
        using handler_type = std::decay_t< Handler >;
        using model_type = detail::basic_op_model< Implementation, decltype(e), handler_type, void(error_code)>;
        model_type *model = model_type::construct(std::move(e), std::forward< Handler >(handler));
        self->impl_.add_waiter(model);
    }
};

BOOST_ASEM_END_NAMESPACE

#endif
