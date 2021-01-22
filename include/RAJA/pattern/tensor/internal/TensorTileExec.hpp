/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file defining SIMD/SIMT register operations.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-19, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_pattern_tensor_TensorTileExec_HPP
#define RAJA_pattern_tensor_TensorTileExec_HPP

#include "RAJA/config.hpp"

#include "RAJA/util/macros.hpp"

#include "RAJA/pattern/tensor/internal/TensorRef.hpp"

namespace RAJA
{


  namespace internal
  {


    template<typename STORAGE, typename DIM_SEQ>
    struct TensorTileExec;

    /**
     * Implement a dimension tiling loop
     */
    template<typename STORAGE, camp::idx_t DIM0, camp::idx_t ... DIM_REST>
    struct TensorTileExec<STORAGE, camp::idx_seq<DIM0, DIM_REST...>>{

      using inner_t = TensorTileExec<STORAGE, camp::idx_seq<DIM_REST...>>;

      template<typename OTILE, typename TTYPE, typename BODY>
      RAJA_HOST_DEVICE
      RAJA_INLINE
      static
      void exec(OTILE const &otile, TTYPE &tile, BODY && body){
        //          printf("store_tile_loop<DIM%d> %d to %d\n",
        //              (int)DIM0, (int)m_ref.m_tile.m_begin[DIM0],
        //              (int)(m_ref.m_tile.m_begin[DIM0] + m_ref.m_tile.m_size[DIM0]));

        auto orig_begin = otile.m_begin[DIM0];
        auto orig_size =  otile.m_size[DIM0];

        // Do the full tile sizes
        for(tile.m_begin[DIM0] = orig_begin;

            tile.m_begin[DIM0] +  STORAGE::s_dim_elem(DIM0) <=
                orig_begin+orig_size;

            tile.m_begin[DIM0] += STORAGE::s_dim_elem(DIM0)){

          // Do the next inner tiling loop
          inner_t::exec(otile, tile, body);
        }

        // Postamble if needed
        if(tile.m_begin[DIM0] <
            orig_begin + orig_size)
        {

          // convert tile to a partial tile
          auto &part_tile = make_tensor_tile_partial(tile);

          // store original size
          auto tmp_size = part_tile.m_size[DIM0];

          // set tile size to the remainder
          part_tile.m_size[DIM0] =
              orig_begin +
              orig_size -
              tile.m_begin[DIM0];

//            printf("store_tile_loop<DIM%d>  postamble %d to %d\n",
//                (int)DIM0, (int)part_tile.m_begin[DIM0],
//                (int)(part_tile.m_size[DIM0] + part_tile.m_size[DIM0]));


          // Do the next inner tiling loop
          inner_t::exec(otile, part_tile, body);

          // restore size
          part_tile.m_size[DIM0] = tmp_size;
        }

        // reset tile dimension
        tile.m_begin[DIM0] = orig_begin;
      }

    };

    /**
     * Termination of nested loop:  execute evaluation of ET
     */
    template<typename STORAGE>
    struct TensorTileExec<STORAGE, camp::idx_seq<>>{

      template<typename OTILE, typename TTYPE, typename BODY>
      RAJA_HOST_DEVICE
      RAJA_INLINE
      static
      void exec(OTILE &, TTYPE const &tile, BODY && body){

        // execute body, passing in the current tile
        body(tile);

      }

    };


    template<typename STORAGE, typename TILE_TYPE, typename BODY, camp::idx_t ... DIM_SEQ>
    RAJA_INLINE
    RAJA_HOST_DEVICE
    void tensorTileExec_expanded(TILE_TYPE const &orig_tile, BODY && body, camp::idx_seq<DIM_SEQ...> const &)
    {

      // tile over full rows and columns
      //tile_type tile{{0,0},{row_tile_size, col_tile_size}};
      TILE_TYPE tile {
        {orig_tile.m_begin[DIM_SEQ]...},
        {STORAGE::s_dim_elem(DIM_SEQ)...},
      };


      // Promote the tile type to a "full-tile" so that the full-element
      // register operations are used.
      // Any of the tiling loops can demote this to a partial-tile when
      // they do postamble execution
      auto &full_tile = make_tensor_tile_full(tile);

      // Do all of the tiling loops

      using tensor_tile_exec_t =
          TensorTileExec<STORAGE, camp::idx_seq<DIM_SEQ...>>;

      tensor_tile_exec_t::exec(orig_tile, full_tile, body);

    }

    template<typename STORAGE, typename TILE_TYPE, typename BODY>
    RAJA_INLINE
    RAJA_HOST_DEVICE
    void tensorTileExec(TILE_TYPE const &tile, BODY && body)
    {
      tensorTileExec_expanded<STORAGE>(tile, body, camp::make_idx_seq_t<STORAGE::s_num_dims>{});
    }

  } // namespace internal

}  // namespace RAJA


#endif
