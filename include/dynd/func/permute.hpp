//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/func/arrfunc.hpp>
#include <dynd/kernels/base_kernel.hpp>
#include <dynd/kernels/cuda_launch.hpp>

namespace dynd {
namespace kernels {

  template <int N, bool with_dst = true>
  struct permute_ck;

  template <int N>
  struct permute_ck<N, true>
      : nd::base_kernel<permute_ck<N, true>, kernel_request_cuda_host_device,
                        N> {
    typedef permute_ck self_type;

    intptr_t perm[N];

    DYND_CUDA_HOST_DEVICE permute_ck(const intptr_t *perm)
    {
      memcpy(this->perm, perm, sizeof(this->perm));
    }

    DYND_CUDA_HOST_DEVICE void single(char *dst, char *const *src)
    {
      char *src_inv_perm[N];
      inv(src_inv_perm, dst, src);

      ckernel_prefix *child = this->get_child_ckernel();
      expr_single_t single = child->get_function<expr_single_t>();
      single(NULL, src_inv_perm, child);
    }

    DYND_CUDA_HOST_DEVICE void strided(char *dst, intptr_t dst_stride,
                                       char *const *src,
                                       const intptr_t *src_stride, size_t count)
    {
      char *src_inv_perm[N];
      inv(src_inv_perm, dst, src);

      intptr_t src_stride_inv_perm[N];
      inv(src_stride_inv_perm, dst_stride, src_stride);

      ckernel_prefix *child = this->get_child_ckernel();
      expr_strided_t strided = child->get_function<expr_strided_t>();
      strided(NULL, 0, src_inv_perm, src_stride_inv_perm, count, child);
    }

    static intptr_t
    instantiate(const arrfunc_type_data *self,
                const ndt::arrfunc_type *DYND_UNUSED(self_tp),
                char *DYND_UNUSED(data), void *ckb, intptr_t ckb_offset,
                const ndt::type &dst_tp, const char *dst_arrmeta, intptr_t nsrc,
                const ndt::type *src_tp, const char *const *src_arrmeta,
                kernel_request_t kernreq, const eval::eval_context *ectx,
                const nd::array &kwds,
                const std::map<nd::string, ndt::type> &tp_vars)
    {
      const std::pair<nd::arrfunc, std::vector<intptr_t>> *data =
          self->get_data_as<std::pair<nd::arrfunc, std::vector<intptr_t>>>();

      const arrfunc_type_data *child = data->first.get();
      const ndt::arrfunc_type *child_tp = data->first.get_type();

      const intptr_t *perm = data->second.data();

      ndt::type src_tp_inv[N];
      inv(src_tp_inv, dst_tp, src_tp, perm);

      const char *src_arrmeta_inv[N];
      inv(src_arrmeta_inv, dst_arrmeta, src_arrmeta, perm);

      self_type::make(ckb, kernreq, ckb_offset,
                      detail::make_array_wrapper<N>(perm));
      return child->instantiate(child, child_tp, NULL, ckb, ckb_offset,
                                ndt::make_type<void>(), NULL, nsrc, src_tp_inv,
                                src_arrmeta_inv, kernreq, ectx, kwds, tp_vars);
    }

  private:
    static void inv(ndt::type *src_inv, const ndt::type &dst,
                    const ndt::type *src, const intptr_t *perm)
    {
      for (intptr_t i = 0; i < N; ++i) {
        intptr_t j = perm[i];
        if (j == -1) {
          src_inv[i] = dst;
        } else {
          src_inv[i] = src[j];
        }
      }
    }

    template <typename T>
    DYND_CUDA_HOST_DEVICE static void inv(T *src_inv, const T &dst,
                                          const T *src, const intptr_t *perm)
    {
      for (intptr_t i = 0; i < N; ++i) {
        intptr_t j = perm[i];
        if (j == -1) {
          src_inv[i] = dst;
        } else {
          src_inv[i] = src[j];
        }
      }
    }

    DYND_CUDA_HOST_DEVICE void inv(ndt::type *src_inv, const ndt::type &dst,
                                   const ndt::type *src)
    {
      return inv(src_inv, dst, src, perm);
    }

    template <typename T>
    DYND_CUDA_HOST_DEVICE void inv(T *src_inv, const T &dst, const T *src)
    {
      return inv(src_inv, dst, src, perm);
    }
  };

  template <int N>
  struct permute_ck<N, false>
      : nd::base_kernel<permute_ck<N, false>, kernel_request_host, N> {
    typedef permute_ck self_type;

    intptr_t perm[N];

    permute_ck(const intptr_t *perm)
    {
      memcpy(this->perm, perm, sizeof(this->perm));
    }

    void single(char *dst, char *const *src)
    {
      char *src_inv_perm[N];
      inv_permute(src_inv_perm, src, perm);

      ckernel_prefix *child = this->get_child_ckernel();
      expr_single_t single = child->get_function<expr_single_t>();
      single(dst, src_inv_perm, child);
    }

    static intptr_t
    instantiate(const arrfunc_type_data *self,
                const ndt::arrfunc_type *DYND_UNUSED(self_tp),
                char *DYND_UNUSED(data), void *ckb, intptr_t ckb_offset,
                const ndt::type &dst_tp, const char *dst_arrmeta, intptr_t nsrc,
                const ndt::type *src_tp, const char *const *src_arrmeta,
                kernel_request_t kernreq, const eval::eval_context *ectx,
                const nd::array &kwds,
                const std::map<nd::string, ndt::type> &tp_vars)
    {
      const std::pair<nd::arrfunc, std::vector<intptr_t>> *data =
          self->get_data_as<std::pair<nd::arrfunc, std::vector<intptr_t>>>();

      const arrfunc_type_data *child = data->first.get();
      const ndt::arrfunc_type *child_tp = data->first.get_type();

      const intptr_t *perm = data->second.data();

      ndt::type src_tp_inv[N];
      inv_permute(src_tp_inv, src_tp, perm);

      const char *src_arrmeta_inv[N];
      inv_permute(src_arrmeta_inv, src_arrmeta, perm);

      self_type::make(ckb, kernreq, ckb_offset,
                      detail::make_array_wrapper<N>(perm));
      return child->instantiate(child, child_tp, NULL, ckb, ckb_offset, dst_tp,
                                dst_arrmeta, nsrc, src_tp_inv, src_arrmeta_inv,
                                kernreq, ectx, kwds, tp_vars);
    }

  private:
    template <typename T>
    static void inv_permute(T *src_inv, const T *src, const intptr_t *perm)
    {
      for (intptr_t i = 0; i < N; ++i) {
        src_inv[i] = src[perm[i]];
      }
    }
  };

} // dynd::kernels

namespace nd {
  namespace functional {

    arrfunc permute(const arrfunc &child, const std::vector<intptr_t> &perm);

  } // namespace dynd::nd::functional
} // namespace dynd::nd
} // namespace dynd