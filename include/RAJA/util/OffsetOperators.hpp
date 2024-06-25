/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file defining Simple Offset Calculators
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-24, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_OFFSETOPERATORS_HPP
#define RAJA_OFFSETOPERATORS_HPP

#include "RAJA/config.hpp"

#include "RAJA/util/concepts.hpp"
#include "RAJA/util/macros.hpp"

namespace RAJA
{

template <typename Ret, typename Arg1 = Ret, typename Arg2 = Arg1>
struct GetOffsetLeft
{
  RAJA_INLINE RAJA_HOST_DEVICE constexpr
  Ret operator()(Arg1 const& i, Arg1 const& num_i,
                 Arg2 const& j, Arg2 const& RAJA_UNUSED_ARG(num_j)) const noexcept
  {
    return i + j * num_i;
  }
};

template <typename Ret, typename Arg1 = Ret, typename Arg2 = Arg1>
struct GetOffsetRight
{
  RAJA_INLINE RAJA_HOST_DEVICE constexpr
  Ret operator()(Arg1 const& i, Arg1 const& RAJA_UNUSED_ARG(num_i),
                 Arg2 const& j, Arg2 const& num_j) const noexcept
  {
    return i * num_j + j;
  }
};

template <size_t t_bunch_num_i,
          typename Ret, typename Arg1 = Ret, typename Arg2 = Arg1>
struct GetOffsetLeftBunched
{
  static constexpr Arg1 bunch_num_i{t_bunch_num_i};

  RAJA_INLINE RAJA_HOST_DEVICE constexpr
  Ret operator()(Arg1 const& i, Arg1 const& RAJA_UNUSED_ARG(num_i),
                 Arg2 const& j, Arg2 const& num_j) const noexcept
  {
    // assert(num_i >= bunch_num_i)
    Arg1 i_inner = i % bunch_num_i;
    Arg1 i_outer = i / bunch_num_i;
    return i_inner + j * bunch_num_i + i_outer * num_j * bunch_num_i;
  }
};

}  // namespace RAJA

#endif
