//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_DETAIL_SEMAPHORE_WAIT_OP_MODEL_HPP
#define BOOST_ASEM_DETAIL_SEMAPHORE_WAIT_OP_MODEL_HPP

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/associated_allocator.hpp>
#include <asio/associated_cancellation_slot.hpp>
#include <asio/executor_work_guard.hpp>
#else
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/executor_work_guard.hpp>
#endif

#include <boost/asem/detail/semaphore_wait_op.hpp>

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{
template < class Executor, class Handler >
struct semaphore_wait_op_model final : semaphore_wait_op
{
    using executor_type = Executor;
    using cancellation_slot_type =
    BOOST_ASEM_ASIO_NAMESPACE::associated_cancellation_slot_t< Handler >;
    using allocator_type = BOOST_ASEM_ASIO_NAMESPACE::associated_allocator_t< Handler >;

    allocator_type
    get_allocator()
    {
        return BOOST_ASEM_ASIO_NAMESPACE::get_associated_allocator(handler_);
    }

    cancellation_slot_type
    get_cancellation_slot()
    {
        return BOOST_ASEM_ASIO_NAMESPACE::get_associated_cancellation_slot(handler_);
    }

    executor_type
    get_executor()
    {
        return work_guard_.get_executor();
    }

    static semaphore_wait_op_model *
    construct(semaphore_base *host, Executor e, Handler handler);

    static void
    destroy(semaphore_wait_op_model *self);

    semaphore_wait_op_model(semaphore_base *host,
                            Executor              e,
                            Handler               handler);

    virtual void
    complete(error_code ec) override;

  private:
    struct cancellation_handler
    {
        semaphore_wait_op_model* self;

        void operator()(asio::cancellation_type type);
    };

  private:
    BOOST_ASEM_ASIO_NAMESPACE::executor_work_guard< Executor > work_guard_;
    Handler                               handler_;
};

}   // namespace detail

BOOST_ASEM_END_NAMESPACE

#endif

#include <boost/asem/detail/impl/semaphore_wait_op_model.hpp>
