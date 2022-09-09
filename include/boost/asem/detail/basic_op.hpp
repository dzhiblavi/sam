//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/asio_experiments
//

#ifndef BOOST_ASEM_DETAIL_SEMAPHORE_WAIT_OP
#define BOOST_ASEM_DETAIL_SEMAPHORE_WAIT_OP

#include <boost/asem/detail/bilist_node.hpp>
#include <boost/asem/detail/config.hpp>

BOOST_ASEM_BEGIN_NAMESPACE


namespace detail
{
template<typename Signature>
struct basic_op;

template<typename ... Ts>
struct basic_op<void(Ts...)> : detail::bilist_node
{
    virtual void complete(Ts...) = 0;
};

using wait_op = basic_op<void(error_code)>;

}   // namespace detail
BOOST_ASEM_END_NAMESPACE

#endif