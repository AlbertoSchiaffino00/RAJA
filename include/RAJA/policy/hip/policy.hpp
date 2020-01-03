/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA HIP policy definitions.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_policy_hip_HPP
#define RAJA_policy_hip_HPP

#include "RAJA/config.hpp"

#if defined(RAJA_ENABLE_HIP)

#include <utility>

#include "RAJA/pattern/reduce.hpp"

#include "RAJA/policy/PolicyBase.hpp"

#include "RAJA/util/Operators.hpp"
#include "RAJA/util/types.hpp"

namespace RAJA
{

using hip_dim_t = dim3;
using hip_dim_member_t = camp::decay<decltype(std::declval<hip_dim_t>().x)>;


//
/////////////////////////////////////////////////////////////////////
//
// Execution policies
//
/////////////////////////////////////////////////////////////////////
//

///
/// Segment execution policies
///

namespace detail
{
template <bool Async>
struct get_launch {
  static constexpr RAJA::Launch value = RAJA::Launch::async;
};

template <>
struct get_launch<false> {
  static constexpr RAJA::Launch value = RAJA::Launch::sync;
};
}  // end namespace detail

namespace policy
{
namespace hip
{

template <size_t BLOCK_SIZE, bool Async = false>
struct hip_exec : public RAJA::make_policy_pattern_launch_platform_t<
                       RAJA::Policy::hip,
                       RAJA::Pattern::forall,
                       detail::get_launch<Async>::value,
                       RAJA::Platform::hip> {
};



//
// NOTE: There is no Index set segment iteration policy for HIP
//

///
///////////////////////////////////////////////////////////////////////
///
/// Reduction reduction policies
///
///////////////////////////////////////////////////////////////////////
///

template <bool maybe_atomic>
struct hip_reduce_base
    : public RAJA::
          make_policy_pattern_launch_platform_t<RAJA::Policy::hip,
                                                RAJA::Pattern::reduce,
                                                detail::get_launch<false>::value,
                                                RAJA::Platform::hip> {
};

using hip_reduce = hip_reduce_base<false>;

using hip_reduce_atomic = hip_reduce_base<true>;


// Policy for RAJA::statement::Reduce that reduces threads in a block
// down to threadIdx 0
struct hip_block_reduce{};

// Policy for RAJA::statement::Reduce that reduces threads in a warp
// down to the first lane of the warp
struct hip_warp_reduce{};

// Policy to map work directly to threads within a warp
// Maximum iteration count is WARP_SIZE
// Cannot be used in conjunction with hip_thread_x_*
// Multiple warps have to be created by using hip_thread_{yz}_*
struct hip_warp_direct{};

// Policy to map work to threads within a warp using a warp-stride loop
// Cannot be used in conjunction with hip_thread_x_*
// Multiple warps have to be created by using hip_thread_{yz}_*
struct hip_warp_loop{};



// Policy to map work to threads within a warp using a bit mask
// Cannot be used in conjunction with hip_thread_x_*
// Multiple warps have to be created by using hip_thread_{yz}_*
// Since we are masking specific threads, multiple nested
// hip_warp_masked
// can be used to create complex thread interleaving patterns
template<typename Mask>
struct hip_warp_masked_direct {};

// Policy to map work to threads within a warp using a bit mask
// Cannot be used in conjunction with hip_thread_x_*
// Multiple warps have to be created by using hip_thread_{yz}_*
// Since we are masking specific threads, multiple nested
// hip_warp_masked
// can be used to create complex thread interleaving patterns
template<typename Mask>
struct hip_warp_masked_loop {};


template<typename Mask>
struct hip_thread_masked_direct {};

template<typename Mask>
struct hip_thread_masked_loop {};




//
// Operations in the included files are parametrized using the following
// values for HIP warp size and max block size.
//
#if defined(__HIPCC__)
constexpr const RAJA::Index_type WARP_SIZE = 64;
#elif defined(__CUDACC__)
constexpr const RAJA::Index_type WARP_SIZE = 32;
#endif

constexpr const RAJA::Index_type MAX_BLOCK_SIZE = 1024;
constexpr const RAJA::Index_type MAX_WARPS = MAX_BLOCK_SIZE / WARP_SIZE;
static_assert(WARP_SIZE >= MAX_WARPS,
              "RAJA Assumption Broken: WARP_SIZE < MAX_WARPS");
static_assert(MAX_BLOCK_SIZE % WARP_SIZE == 0,
              "RAJA Assumption Broken: MAX_BLOCK_SIZE not "
              "a multiple of WARP_SIZE");

struct hip_synchronize : make_policy_pattern_launch_t<Policy::hip,
                                                       Pattern::synchronize,
                                                       Launch::sync> {
};

}  // end namespace hip
}  // end namespace policy

using policy::hip::hip_exec;

template <size_t BLOCK_SIZE>
using hip_exec_async = policy::hip::hip_exec<BLOCK_SIZE, true>;

using policy::hip::hip_reduce_base;
using policy::hip::hip_reduce;
using policy::hip::hip_reduce_atomic;

using policy::hip::hip_block_reduce;
using policy::hip::hip_warp_reduce;

using policy::hip::hip_warp_direct;
using policy::hip::hip_warp_loop;

using policy::hip::hip_warp_masked_direct;
using policy::hip::hip_warp_masked_loop;

using policy::hip::hip_thread_masked_direct;
using policy::hip::hip_thread_masked_loop;

using policy::hip::hip_synchronize;




/*!
 * Maps segment indices to HIP threads.
 * This is the lowest overhead mapping, but requires that there are enough
 * physical threads to fit all of the direct map requests.
 * For example, a segment of size 2000 will not fit, and trigger a runtime
 * error.
 */
template<int dim>
struct hip_thread_xyz_direct{};

using hip_thread_x_direct = hip_thread_xyz_direct<0>;
using hip_thread_y_direct = hip_thread_xyz_direct<1>;
using hip_thread_z_direct = hip_thread_xyz_direct<2>;


/*!
 * Maps segment indices to HIP threads.
 * Uses block-stride looping to exceed the maximum number of physical threads
 */
template<int dim, int min_threads>
struct hip_thread_xyz_loop{};

using hip_thread_x_loop = hip_thread_xyz_loop<0, 1>;
using hip_thread_y_loop = hip_thread_xyz_loop<1, 1>;
using hip_thread_z_loop = hip_thread_xyz_loop<2, 1>;


/*!
 * Maps segment indices to HIP blocks.
 * Uses grid-stride looping to exceed the maximum number of blocks
 */
template<int dim>
struct hip_block_xyz_loop{};

using hip_block_x_loop = hip_block_xyz_loop<0>;
using hip_block_y_loop = hip_block_xyz_loop<1>;
using hip_block_z_loop = hip_block_xyz_loop<2>;




namespace internal{

template<int dim>
struct HipDimHelper;

template<>
struct HipDimHelper<0>{

  inline
  static
  constexpr
  RAJA_HOST_DEVICE
  auto get(hip_dim_t const &d) ->
    decltype(d.x)
  {
    return d.x;
  }

  inline
  static
  RAJA_HOST_DEVICE
  void set(hip_dim_t &d, int value)
  {
    d.x = value;
  }
};

template<>
struct HipDimHelper<1>{

  inline
  static
  constexpr
  RAJA_HOST_DEVICE
  auto get(hip_dim_t const &d) ->
    decltype(d.x)
  {
    return d.y;
  }

  inline
  static
  RAJA_HOST_DEVICE
  void set(hip_dim_t &d, int value)
  {
    d.y = value;
  }
};

template<>
struct HipDimHelper<2>{

  inline
  static
  constexpr
  RAJA_HOST_DEVICE
  auto get(hip_dim_t const &d) ->
    decltype(d.x)
  {
    return d.z;
  }

  inline
  static
  RAJA_HOST_DEVICE
  void set(hip_dim_t &d, int value)
  {
    d.z = value;
  }
};

template<int dim>
constexpr
RAJA_HOST_DEVICE
auto get_hip_dim(hip_dim_t const &d) ->
  decltype(d.x)
{
  return HipDimHelper<dim>::get(d);
}

template<int dim>
RAJA_HOST_DEVICE
void set_hip_dim(hip_dim_t &d, int value)
{
  return HipDimHelper<dim>::set(d, value);
}
} // namespace internal


}  // namespace RAJA

#endif  // RAJA_ENABLE_HIP
#endif
