/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file defining a SIMD register abstraction.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-19, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifdef __AVX512F__

#ifndef RAJA_policy_vector_register_avx512_long_HPP
#define RAJA_policy_vector_register_avx512_long_HPP

#include "RAJA/config.hpp"
#include "RAJA/util/macros.hpp"
#include "RAJA/pattern/simd_register/Register.hpp"

// Include SIMD intrinsics header file
#include <immintrin.h>
#include <cmath>


namespace RAJA
{

  template<int SKEW>
  class Register<avx512_register, long, SKEW> :
    public internal::RegisterBase<Register<avx512_register, long, SKEW>>
  {
    public:
      using register_policy = avx512_register;
      using self_type = Register<avx512_register, long, SKEW>;
      using element_type = long;
      using register_type = __m512i;


    private:
      register_type m_value;

      RAJA_INLINE
      __mmask8 createMask(camp::idx_t N) const {
        // Generate a mask
				switch(N){
					case 0: return __mmask8(0x00);
					case 1: return __mmask8(0x01);
					case 2: return __mmask8(0x03);
					case 3: return __mmask8(0x07);
					case 4: return __mmask8(0x0F);
					case 5: return __mmask8(0x1F);
					case 6: return __mmask8(0x3F);
					case 7: return __mmask8(0x7F);
					case 8: return __mmask8(0xFF);
				}
				return __mmask8(0);
      }

      RAJA_INLINE
      __m512i createStridedOffsets(camp::idx_t stride) const {
        // Generate a strided offset list
				auto vstride = _mm512_set1_epi64(stride);
				auto vseq = _mm512_set_epi64(7, 6, 5, 4, 3, 2, 1, 0);
				return _mm512_mullo_epi64(vstride, vseq);
      }

    public:

      static constexpr camp::idx_t s_num_elem = 8;

      /*!
       * @brief Default constructor, zeros register contents
       */
			// AVX512F
      RAJA_INLINE
      Register() : m_value(_mm512_setzero_epi32()) {
      }

      /*!
       * @brief Copy constructor from underlying simd register
       */
      RAJA_INLINE
      constexpr
      explicit Register(register_type const &c) : m_value(c) {}


      /*!
       * @brief Copy constructor
       */
      RAJA_INLINE
      constexpr
      Register(self_type const &c) : m_value(c.m_value) {}

      /*!
       * @brief Copy assignment constructor
       */
      RAJA_INLINE
      self_type &operator=(self_type const &c){
        m_value = c.m_value;
        return *this;
      }

      /*!
       * @brief Construct from scalar.
       * Sets all elements to same value (broadcast).
       */
			// AVX512F
      RAJA_INLINE
      Register(element_type const &c) : m_value(_mm512_set1_epi64(c)) {}


      /*!
       * @brief Load a full register from a stride-one memory location
       *
       */
      RAJA_INLINE
      self_type &load_packed(element_type const *ptr){
			  // AVX512F
        m_value = _mm512_loadu_epi64(ptr);
        return *this;
      }

      /*!
       * @brief Partially load a register from a stride-one memory location given
       *        a run-time number of elements.
       *
       */
      RAJA_INLINE
      self_type &load_packed_n(element_type const *ptr, camp::idx_t N){
			  // AVX512F
        m_value = _mm512_mask_loadu_epi64(_mm512_setzero_epi32(), createMask(N), ptr);
        return *this;
      }

      /*!
       * @brief Gather a full register from a strided memory location
       *
       */
      RAJA_INLINE
      self_type &load_strided(element_type const *ptr, camp::idx_t stride){
			  // AVX512F
        m_value = _mm512_i64gather_epi64(createStridedOffsets(stride),
				                              ptr,
                                      sizeof(element_type));
        return *this;
      }


      /*!
       * @brief Partially load a register from a stride-one memory location given
       *        a run-time number of elements.
       *
       */
      RAJA_INLINE
      self_type &load_strided_n(element_type const *ptr, camp::idx_t stride, camp::idx_t N){
				// AVX512F
        m_value = _mm512_mask_i64gather_epi64(_mm512_setzero_epi32(),
                                      createMask(N),
                                      createStridedOffsets(stride),
                                      ptr,
                                      sizeof(element_type));
        return *this;
      }


      /*!
       * @brief Store entire register to consecutive memory locations
       *
       */
      RAJA_INLINE
      self_type const &store_packed(element_type *ptr) const{
				// AVX512F
        _mm512_storeu_epi64(ptr, m_value);
        return *this;
      }

      /*!
       * @brief Store entire register to consecutive memory locations
       *
       */
      RAJA_INLINE
      self_type const &store_packed_n(element_type *ptr, camp::idx_t N) const{
				// AVX512F
        _mm512_mask_storeu_epi64(ptr, createMask(N), m_value);
        return *this;
      }

      /*!
       * @brief Store entire register to consecutive memory locations
       *
       */
      RAJA_INLINE
      self_type const &store_strided(element_type *ptr, camp::idx_t stride) const{
				// AVX512F
				_mm512_i64scatter_epi64(ptr,
				                     createStridedOffsets(stride),
														 m_value,
														 sizeof(element_type));
        return *this;
      }


      /*!
       * @brief Store partial register to consecutive memory locations
       *
       */
      RAJA_INLINE
      self_type const &store_strided_n(element_type *ptr, camp::idx_t stride, camp::idx_t N) const{
				// AVX512F
				_mm512_mask_i64scatter_epi64(ptr,
                           				createMask(N),
				                          createStridedOffsets(stride),
																	m_value,
														      sizeof(element_type));
        return *this;
      }

      /*!
       * @brief Get scalar value from vector register
       * @param i Offset of scalar to get
       * @return Returns scalar value at i
       */
      RAJA_INLINE
      element_type get(camp::idx_t i) const
      {return m_value[i];}


      /*!
       * @brief Set scalar value in vector register
       * @param i Offset of scalar to set
       * @param value Value of scalar to set
       */
      RAJA_INLINE
      self_type &set(camp::idx_t i, element_type value)
      {
        m_value[i] = value;
        return *this;
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type &broadcast(element_type const &value){
        m_value =  _mm512_set1_epi64(value);
        return *this;
      }


      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type &copy(self_type const &src){
        m_value = src.m_value;
        return *this;
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type add(self_type const &b) const {
        return self_type(_mm512_add_epi64(m_value, b.m_value));
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type subtract(self_type const &b) const {
        return self_type(_mm512_sub_epi64(m_value, b.m_value));
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type multiply(self_type const &b) const {
        return self_type(_mm512_mullo_epi64(m_value, b.m_value));
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type divide(self_type const &b, camp::idx_t N = 8) const {
        // AVX512 does not supply an integer divide, so do it manually
        return self_type(_mm512_set_epi64(
            N >= 8 ? get(7)/b.get(7) : 0,
            N >= 7 ? get(6)/b.get(6) : 0,
            N >= 6 ? get(5)/b.get(5) : 0,
            N >= 5 ? get(4)/b.get(4) : 0,
            N >= 4 ? get(3)/b.get(3) : 0,
            N >= 3 ? get(2)/b.get(2) : 0,
            N >= 2 ? get(1)/b.get(1) : 0,
            N >= 1 ? get(0)/b.get(0) : 0
            ));
      }


      /*!
       * @brief Sum the elements of this vector
       * @return Sum of the values of the vectors scalar elements
       */
      RAJA_INLINE
      element_type sum(camp::idx_t N = 8) const
      {
				return _mm512_mask_reduce_add_epi64(createMask(N), m_value);
      }


      /*!
       * @brief Returns the largest element
       * @return The largest scalar element in the register
       */
      RAJA_INLINE
      element_type max(camp::idx_t N = 8) const
      {
				return _mm512_mask_reduce_max_epi64(createMask(N), m_value);
      }

      /*!
       * @brief Returns element-wise largest values
       * @return Vector of the element-wise max values
       */
      RAJA_INLINE
      self_type vmax(self_type a) const
      {
        return self_type(_mm512_max_epi64(m_value, a.m_value));
      }

      /*!
       * @brief Returns the largest element
       * @return The largest scalar element in the register
       */
      RAJA_INLINE
      element_type min(camp::idx_t N = 8) const
      {
				return _mm512_mask_reduce_min_epi64(createMask(N), m_value);
      }

      /*!
       * @brief Returns element-wise largest values
       * @return Vector of the element-wise max values
       */
      RAJA_INLINE
      self_type vmin(self_type a) const
      {
        return self_type(_mm512_min_epi64(m_value, a.m_value));
      }
  };



}  // namespace RAJA


#endif

#endif //__AVX512F__
