//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <map>
#include <memory>

#include <dynd/callables/apply_callable_callable.hpp>
#include <dynd/dispatcher.hpp>
#include <dynd/type_registry.hpp>

namespace dynd {
namespace nd {
  namespace detail {

    /**
     * Returns a reference to the map of registered callables.
     * NOTE: The internal representation will change, this
     *       function will change.
     */
    DYND_API std::map<std::string, callable> &get_regfunctions();

    /**
     * Presently, there are some specially treated keyword arguments in
     * arrfuncs. The "dst_tp" keyword argument always tells the desired
     * output type, and the "dst" keyword argument always provides an
     * output array.
     */
    inline bool is_special_kwd(const ndt::callable_type *DYND_UNUSED(self_tp), array &dst, const std::string &name,
                               const nd::array &value) {
      if (name == "dst_tp") {
        dst = nd::empty(value.as<ndt::type>());
        return true;
      } else if (name == "dst") {
        dst = value;
        return true;
      }

      return false;
    }

    DYND_API void check_narg(const ndt::callable_type *af_tp, intptr_t narg);

    DYND_API void check_arg(const ndt::callable_type *af_tp, intptr_t i, const ndt::type &actual_tp,
                            const char *actual_arrmeta, std::map<std::string, ndt::type> &tp_vars);

    template <template <type_id_t...> class KernelType>
    struct make_all;

    template <template <type_id_t...> class KernelType>
    struct new_make_all;

    template <template <type_id_t...> class KernelType, template <type_id_t...> class Condition>
    struct make_all_if;

  } // namespace dynd::nd::detail

  typedef array callable_arg_t;
  typedef std::pair<const char *, array> callable_kwd_t;

  /**
   * Holds a single instance of a callable in an nd::array,
   * providing some more direct convenient interface.
   */
  class DYND_API callable : public intrusive_ptr<base_callable> {
  public:
    using intrusive_ptr<base_callable>::intrusive_ptr;

    callable() = default;

    template <typename CallableType, typename... T, typename = std::enable_if_t<all_char_string_params<T...>::value>>
    callable(CallableType f, T &&... names)
        : callable(new functional::apply_callable_callable<CallableType, arity_of<CallableType>::value - sizeof...(T)>(
                       f, std::forward<T>(names)...),
                   true) {}

    bool is_null() const { return get() == NULL; }

    callable_property get_flags() const { return right_associative; }

    const ndt::callable_type *get_type() const {
      if (get() == NULL) {
        return NULL;
      }

      return m_ptr->get_type().extended<ndt::callable_type>();
    }

    ndt::type resolve(const ndt::type &dst_tp, size_t nsrc, const ndt::type *src_tp, size_t nkwd, const array *kwds) {
      std::map<std::string, ndt::type> tp_vars;

      call_graph cg;
      return m_ptr->resolve(nullptr, nullptr, cg, dst_tp, nsrc, src_tp, nkwd, kwds, tp_vars);
    }

    ndt::type resolve(const ndt::type &dst_tp, std::initializer_list<ndt::type> src_tp,
                      std::initializer_list<array> kwds) {
      return resolve(dst_tp, src_tp.size(), src_tp.begin(), kwds.size(), kwds.begin());
    }

    ndt::type resolve(std::initializer_list<ndt::type> src_tp, std::initializer_list<array> kwds) {
      return resolve(get_type()->get_return_type(), src_tp.size(), src_tp.begin(), kwds.size(), kwds.begin());
    }

    const ndt::type &get_array_type() const { return m_ptr->get_type(); }

    const ndt::type &get_ret_type() const { return get_type()->get_return_type(); }

    std::intptr_t get_narg() const { return get_type()->get_npos(); }

    const ndt::type &get_arg_type(std::intptr_t i) const { return get_type()->get_pos_type(i); }

    const std::vector<ndt::type> &get_arg_types() const { return get_type()->get_pos_types(); }

    void overload(const ndt::type &ret_tp, intptr_t narg, const ndt::type *arg_tp, const callable &value) {
      get()->overload(ret_tp, narg, arg_tp, value);
    }

    void overload(const ndt::type &ret_tp, const std::initializer_list<ndt::type> &arg_tp, const callable &value) {
      overload(ret_tp, arg_tp.size(), arg_tp.begin(), value);
    }

    const callable &specialize(const ndt::type &ret_tp, intptr_t narg, const ndt::type *arg_tp) const {
      return get()->specialize(ret_tp, narg, arg_tp);
    }

    const callable &specialize(const ndt::type &ret_tp, const std::initializer_list<ndt::type> &arg_tp) const {
      return specialize(ret_tp, arg_tp.size(), arg_tp.begin());
    }

    array call(size_t args_size, const array *args_values, size_t kwds_size,
               const std::pair<const char *, array> *kwds_values) const;

    template <typename... ArgTypes>
    array operator()(ArgTypes &&... args) const {
      array tmp[sizeof...(ArgTypes)] = {std::forward<ArgTypes>(args)...};
      return call(sizeof...(ArgTypes), tmp, 0, nullptr);
    }

    array operator()() const { return call(0, nullptr, 0, nullptr); }

    array operator()(std::initializer_list<std::pair<const char *, array>> kwds) const {
      return call(0, nullptr, kwds.size(), kwds.begin());
    }

    array operator()(const std::initializer_list<array> &args,
                     const std::initializer_list<std::pair<const char *, array>> &kwds) const {
      return call(args.size(), args.begin(), kwds.size(), kwds.begin());
    }

    template <template <type_id_t> class KernelType, typename I0, typename... A>
    static dispatcher<callable> new_make_all(A &&... a) {
      std::vector<std::pair<std::vector<type_id_t>, callable>> callables;
      for_each<I0>(detail::new_make_all<KernelType>(), callables, std::forward<A>(a)...);

      return dispatcher<callable>(callables.begin(), callables.end());
    }

    template <template <type_id_t> class KernelType, typename I0, typename... A>
    static std::map<type_id_t, callable> make_all(A &&... a) {
      std::map<type_id_t, callable> callables;
      for_each<I0>(detail::make_all<KernelType>(), callables, std::forward<A>(a)...);

      return callables;
    }

    template <template <type_id_t, type_id_t, type_id_t...> class KernelType, typename I0, typename I1, typename... I,
              typename... A>
    static std::map<std::array<type_id_t, 2 + sizeof...(I)>, callable> make_all(A &&... a) {
      std::map<std::array<type_id_t, 2 + sizeof...(I)>, callable> callables;
      for_each<typename outer<I0, I1, I...>::type>(detail::make_all<KernelType>(), callables, std::forward<A>(a)...);

      return callables;
    }

    template <template <type_id_t, type_id_t, type_id_t...> class KernelType, typename I0, typename I1, typename... I,
              typename... A>
    static dispatcher<callable> new_make_all(A &&... a) {
      std::vector<std::pair<std::vector<type_id_t>, callable>> callables;
      for_each<typename outer<I0, I1, I...>::type>(detail::new_make_all<KernelType>(), callables,
                                                   std::forward<A>(a)...);

      return dispatcher<callable>(callables.begin(), callables.end());
    }

    template <template <type_id_t> class KernelType, template <type_id_t> class Condition, typename I0, typename... A>
    static dispatcher<callable> make_all_if(A &&... a) {
      std::vector<std::pair<std::vector<type_id_t>, callable>> callables;
      for_each<I0>(detail::make_all_if<KernelType, Condition>(), callables, std::forward<A>(a)...);

      return dispatcher<callable>(callables.begin(), callables.end());
    }

    template <template <type_id_t, type_id_t, type_id_t...> class KernelType,
              template <type_id_t, type_id_t, type_id_t...> class Condition, typename I0, typename I1, typename... I,
              typename... A>
    static dispatcher<callable> make_all_if(A &&... a) {
      std::vector<std::pair<std::vector<type_id_t>, callable>> callables;
      for_each<typename outer<I0, I1, I...>::type>(detail::make_all_if<KernelType, Condition>(), callables,
                                                   std::forward<A>(a)...);

      return dispatcher<callable>(callables.begin(), callables.end());
    }
  };

  template <typename CallableType, typename... ArgTypes>
  std::enable_if_t<std::is_base_of<base_callable, CallableType>::value, callable> make_callable(ArgTypes &&... args) {
    return callable(new CallableType(std::forward<ArgTypes>(args)...), true);
  }

  template <typename KernelType, typename... ArgTypes>
  std::enable_if_t<std::is_base_of<base_kernel<KernelType>, KernelType>::value, callable>
  make_callable(const ndt::type &tp) {
    return make_callable<default_instantiable_callable<KernelType>>(tp);
  }

  inline std::ostream &operator<<(std::ostream &o, const callable &rhs) {
    return o << "<callable <" << rhs->get_type() << "> at " << reinterpret_cast<const void *>(rhs.get()) << ">";
  }

  namespace detail {

    template <template <type_id_t...> class KernelType, typename S>
    struct apply;

    template <template <type_id_t...> class KernelType, type_id_t... I>
    struct apply<KernelType, type_id_sequence<I...>> {
      typedef KernelType<I...> type;
    };

    template <template <type_id_t...> class KernelType>
    struct make_all {
      template <type_id_t TypeID, typename... A>
      void on_each(std::map<type_id_t, callable> &callables, A &&... a) const {
        callables[TypeID] = make_callable<KernelType<TypeID>>(std::forward<A>(a)...);
      }

      template <typename TypeIDSequence, typename... A>
      void on_each(std::map<std::array<type_id_t, TypeIDSequence::size2()>, callable> &callables, A &&... a) const {
        callables[i2a<TypeIDSequence>()] =
            make_callable<typename apply<KernelType, TypeIDSequence>::type>(std::forward<A>(a)...);
      }
    };

    template <template <type_id_t...> class KernelType>
    struct new_make_all {
      template <type_id_t TypeID, typename... A>
      void on_each(std::vector<std::pair<std::vector<type_id_t>, callable>> &callables, A &&... a) const {
        callables.push_back({{TypeID}, make_callable<KernelType<TypeID>>(std::forward<A>(a)...)});
      }

      template <typename TypeIDSequence, typename... A>
      void on_each(std::vector<std::pair<std::vector<type_id_t>, callable>> &callables, A &&... a) const {
        auto arr = i2a<TypeIDSequence>();
        std::vector<type_id_t> v(arr.size());
        for (size_t i = 0; i < arr.size(); ++i) {
          v[i] = arr[i];
        }

        callables.push_back(
            {{v}, make_callable<typename apply<KernelType, TypeIDSequence>::type>(std::forward<A>(a)...)});
      }
    };

    template <template <type_id_t...> class KernelType, template <type_id_t...> class Condition>
    struct make_all_if {
      template <type_id_t TypeID, typename... A>
      void on_each(std::vector<std::pair<std::vector<type_id_t>, callable>> &callables, A &&... a) const {
        if (Condition<TypeID>::value) {
          callables.push_back({{TypeID}, make_callable<KernelType<TypeID>>(std::forward<A>(a)...)});
        }
      }

      template <typename TypeIDSequence, typename... A>
      void on_each(std::vector<std::pair<std::vector<type_id_t>, callable>> &callables, A &&... a) const {
        if (apply<Condition, TypeIDSequence>::type::value) {
          auto arr = i2a<TypeIDSequence>();
          std::vector<type_id_t> v;
          for (size_t i = 0; i < arr.size(); ++i) {

            v.push_back(arr[i]);
          }

          callables.push_back(
              {{v}, make_callable<typename apply<KernelType, TypeIDSequence>::type>(std::forward<A>(a)...)});
        }
      }
    };

  } // namespace dynd::nd::detail

  DYND_API std::map<std::string, callable> &callables();

} // namespace dynd::nd

/**
 * Creates a callable which does the assignment from
 * data of src_tp to dst_tp.
 *
 * \param dst_tp  The type of the destination.
 * \param src_tp  The type of the source.
 * \param errmode  The error mode to use for the assignment.
 */
DYND_API nd::callable make_callable_from_assignment(const ndt::type &dst_tp, const ndt::type &src_tp,
                                                    assign_error_mode errmode);

} // namespace dynd
