//
// Copyright (C) 2011-14 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DYND__LIFT_CKERNEL_DEFERRED_HPP_
#define _DYND__LIFT_CKERNEL_DEFERRED_HPP_

#include <dynd/config.hpp>
#include <dynd/array.hpp>
#include <dynd/func/arrfunc.hpp>

namespace dynd {

/**
 * Lifts the provided ckernel, broadcasting it as necessary to execute
 * across the additional dimensions in the ``lifted_types`` array.
 *
 * \param out_ckd  The output arrfunc which is filled.
 * \param ckd  The arrfunc to be lifted.
 * \param lifted_types  The types to lift the arrfunc to. The output arrfunc
 *                      is for these types.
 */
void lift_ckernel_deferred(arrfunc *out_ckd,
                const nd::array& ckd,
                const std::vector<ndt::type>& lifted_types);

} // namespace dynd

#endif // _DYND__LIFT_CKERNEL_DEFERRED_HPP_
