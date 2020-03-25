/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA simd policy definitions.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-19, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_policy_vector_register_HPP
#define RAJA_policy_vector_register_HPP

#include "RAJA/config.hpp"
#include<RAJA/pattern/register.hpp>
#include<RAJA/policy/vector/policy.hpp>


#ifdef __AVX2__
#include<RAJA/policy/vector/register/avx2.hpp>
#ifndef RAJA_VECTOR_REGISTER_TYPE
#define RAJA_VECTOR_REGISTER_TYPE RAJA::vector_avx2_register
#endif
#endif


#ifdef __AVX__
#include<RAJA/policy/vector/register/avx.hpp>
#ifndef RAJA_VECTOR_REGISTER_TYPE
#define RAJA_VECTOR_REGISTER_TYPE RAJA::vector_avx_register
#endif
#endif



#ifdef RAJA_ALTIVEC
#include<RAJA/policy/vector/register/altivec.hpp>
#ifndef RAJA_VECTOR_REGISTER_TYPE
#define RAJA_VECTOR_REGISTER_TYPE RAJA::vector_altivec_register
#endif
#endif


// The scalar register is always supported (doesn't require any SIMD/SIMT)
#include<RAJA/policy/vector/register/scalar.hpp>
#ifndef RAJA_VECTOR_REGISTER_TYPE
#define RAJA_VECTOR_REGISTER_TYPE RAJA::vector_scalar_register
#endif


namespace RAJA
{
namespace policy
{
  namespace vector
  {

    // This sets the default SIMD register that will be used
    using default_register_type = RAJA_VECTOR_REGISTER_TYPE;
  }
}



  template<typename T, size_t UNROLL = 1, typename REGISTER = policy::vector::default_register_type>
  using StreamVector = StreamVectorExt<
      RAJA::Register<REGISTER, T, RAJA::RegisterTraits<REGISTER, T>::s_num_elem>,
      UNROLL>;

  template<typename T, size_t NumElem, typename REGISTER = policy::vector::default_register_type>
  using FixedVector = FixedVectorExt<
      RAJA::Register<REGISTER, T, RAJA::RegisterTraits<REGISTER, T>::s_num_elem>,
      NumElem>;

}

#endif
