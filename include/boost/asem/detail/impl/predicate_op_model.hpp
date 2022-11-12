//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_DETAIL_IMPL_PREDICATE_OP_MODEL_HPP
#define BOOST_ASEM_DETAIL_IMPL_PREDICATE_OP_MODEL_HPP

#include <boost/asem/detail/predicate_op_model.hpp>
#include <exception>

#if defined(BOOST_ASEM_STANDALONE)
#include <asio/experimental/append.hpp>
#include <asio/post.hpp>
#else
#include <boost/asio/experimental/append.hpp>
#include <boost/asio/post.hpp>
#endif

BOOST_ASEM_BEGIN_NAMESPACE

namespace detail
{
template < class Implementation, class Executor, class Handler, class Predicate, class ... Ts >
auto
predicate_op_model< Implementation, Executor, Handler, Predicate, void(error_code ec, Ts...)>::construct(
    Executor              e,
    Handler               handler,
    Predicate             predicate)
    -> predicate_op_model *
{
    auto halloc = net::get_associated_allocator(handler);
    auto alloc  = typename std::allocator_traits< decltype(halloc) >::
        template rebind_alloc< predicate_op_model >(halloc);
    using traits = std::allocator_traits< decltype(alloc) >;
    auto pmem   = traits::allocate(alloc, 1);

    struct dealloc
    {
        ~dealloc()
        {
#if defined(__cpp_lib_uncaught_exceptions)
            if (std::uncaught_exceptions() > 0)
#else
            if (std::uncaught_exception())
#endif
                traits::deallocate(alloc_, pmem_, 1);
        }
        decltype(alloc) alloc_;
        decltype(pmem) pmem_;
    };

    dealloc dc{halloc, pmem};
    return new (pmem)
            predicate_op_model(std::move(e), std::move(handler), std::move(predicate));
}

template < class Implementation, class Executor, class Handler, class Predicate, class ... Ts >
auto
predicate_op_model< Implementation, Executor, Handler, Predicate, void(error_code ec, Ts...) >::destroy(
        predicate_op_model *self) -> void
{
    auto halloc = self->get_allocator();
    auto alloc  = typename std::allocator_traits< decltype(halloc) >::
        template rebind_alloc< predicate_op_model >(halloc);
    self->~predicate_op_model();
    auto traits = std::allocator_traits< decltype(alloc) >();
    traits.deallocate(alloc, self, 1);
}

template < class Implementation, class Executor, class Handler, class Predicate, class ... Ts >
predicate_op_model< Implementation, Executor, Handler, Predicate, void(error_code ec, Ts...) >::predicate_op_model(
    Executor              e,
    Handler               handler,
    Predicate             predicate)
: work_guard_(std::move(e))
, handler_(std::move(handler))
, predicate_(std::move(predicate))
{
    auto slot = get_cancellation_slot();
    if (slot.is_connected())
        slot.assign(
            [this](net::cancellation_type type)
            {
                if (type != net::cancellation_type::none)
                {
                    predicate_op_model *self = this;
                    self->complete(net::error::operation_aborted);
                }
            });
}

template < class Implementation, class Executor, class Handler, class Predicate, class ... Ts >
void
predicate_op_model< Implementation, Executor, Handler, Predicate, void(error_code ec, Ts...) >::complete(error_code ec, Ts ... args)
{
    get_cancellation_slot().clear();
    auto g = std::move(work_guard_);
    auto h = std::move(handler_);
    this->unlink();
    destroy(this);
    net::post(g.get_executor(),
                                    net::append(std::move(h), ec, std::move(args)...));
}

template < class Implementation, class Executor, class Handler, class Predicate, class ... Ts >
void
predicate_op_model< Implementation, Executor, Handler, Predicate, void(error_code ec, Ts...) >::shutdown()
{
  get_cancellation_slot().clear();
  this->unlink();
  destroy(this);
}

}   // namespace detail

BOOST_ASEM_END_NAMESPACE

#endif /// BOOST_ASEM_DETAIL_IMPL_PREDICATE_OP_MODEL_HPP
