// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_
#  error Please include <asteria/llds/avmc_queue.hpp> instead.
#endif
namespace asteria {
namespace details_avmc_queue {

// This union can be used to encapsulate trivial information in solidified
// nodes. Each field is assigned a unique suffix. At most 48 bits can be
// stored here. You may make appropriate use of them.
union Uparam
  {
    struct {
      char _x1[2];
      bool b0, b1, b2, b3, b4, b5;
    };
    struct {
      char _x2[2];
      char c0, c1, c2, c3, c4, c5;
    };
    struct {
      char _x3[2];
      int8_t i0, i1, i2, i3, i4, i5;
    };
    struct {
      char _x4[2];
      uint8_t u0, u1, u2, u3, u4, u5;
    };
    struct {
      char _x5[2];
      int16_t i01, i23, i45;
    };
    struct {
      char _x6[2];
      uint16_t u01, u23, u45;
    };
    struct {
      char _x7[4];
      int32_t i2345;
    };
    struct {
      char _x8[4];
      uint32_t u2345;
    };
  };

struct Metadata;
struct Header;

// These are prototypes for callbacks.
using Executor     = AIR_Status (Executive_Context& ctx, const Header* head);
using Constructor  = void (Header* head, intptr_t arg);
using Destructor   = void (Header* head);
using Var_Getter   = void (Variable_HashMap& staged, Variable_HashMap& temp, const Header* head);

struct Metadata
  {
    // Version 1
    Executor* exec;        // executor function, must not be null
    Destructor* dtor_opt;  // if null then no cleanup is performed
    Var_Getter* vget_opt;  // if null then no variable shall exist

    // Version 2
    Source_Location sloc;  // symbols
  };

// This is the header of each variable-length element that is stored in an AVMC
// queue. User-defined data (the `sparam`) may follow this struct, so the size
// of this struct has to be a multiple of `alignof(max_align_t)`.
struct Header
  {
    union {
      struct {
        uint8_t nheaders;  // size of `sparam`, in number of headers [!]
        uint8_t meta_ver;  // version of `Metadata`; `pv_meta` active
      };
      Uparam uparam;
    };

    union {
      Executor* pv_exec;  // executor function
      Metadata* pv_meta;
    };

    alignas(max_align_t) char sparam[];
  };

template<typename SparamT>
inline
void
do_nontrivial_dtor(Header* head)
  {
    auto ptr = reinterpret_cast<SparamT*>(head->sparam);
    ::rocket::destroy(ptr);
  }

template<typename SparamT>
inline
void
do_call_get_variables(Variable_HashMap& staged, Variable_HashMap& temp,
                      const Header* head)
  {
    auto ptr = reinterpret_cast<const SparamT*>(head->sparam);
    ptr->collect_variables(staged, temp);
  }

template<typename SparamT, typename = void>
struct select_get_variables
  {
    constexpr operator
    Var_Getter*() const noexcept
      { return nullptr;  }
  };

template<typename SparamT>
struct select_get_variables<SparamT,
    ROCKET_VOID_DECLTYPE(
      ::std::declval<const SparamT&>().collect_variables(
          ::std::declval<Variable_HashMap&>(),  // staged
          ::std::declval<Variable_HashMap&>()   // temp
        ))>
  {
    constexpr operator
    Var_Getter*() const noexcept
      { return do_call_get_variables<SparamT>;  }
  };

template<typename SparamT>
struct Sparam_traits
  {
    static constexpr Destructor* dtor_opt =
        ::std::is_trivially_destructible<SparamT>::value
            ? nullptr
            : do_nontrivial_dtor<SparamT>;

    static constexpr Var_Getter* vget_opt =
        select_get_variables<SparamT>();
  };

template<typename XSparamT>
inline
void
do_forward_ctor(Header* head, intptr_t arg)
  {
    using Sparam = typename ::std::decay<XSparamT>::type;
    auto ptr = reinterpret_cast<Sparam*>(head->sparam);
    auto src = reinterpret_cast<Sparam*>(arg);
    ::rocket::construct(ptr, static_cast<XSparamT&&>(*src));
  }

}  // namespace details_avmc_queue
}  // namespace asteria
