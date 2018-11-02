// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UTILITIES_HPP_
#define ROCKET_UTILITIES_HPP_

#include <type_traits> // std::common_type<>
#include <iterator> // std::iterator_traits<>
#include <utility> // std::swap(), std::move(), std::forward()
#include <new> // placement new
#include <initializer_list> // std::initializer_list<>
#include <ios> // std::ios_base, std::basic_ios<>
#include <cstddef> // std::size_t, std::ptrdiff_t

namespace rocket {

    namespace noadl = ::rocket;

using ::std::common_type;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_default_constructible;
using ::std::is_nothrow_destructible;
using ::std::underlying_type;
using ::std::conditional;
using ::std::false_type;
using ::std::true_type;
using ::std::integral_constant;
using ::std::iterator_traits;
using ::std::initializer_list;
using ::std::ios_base;
using ::std::basic_ios;
using ::std::size_t;
using ::std::ptrdiff_t;

#define ROCKET_COPYABLE_DESTRUCTOR(c_, ...)  \
    c_(const c_ &)              = default;  \
    c_ & operator=(const c_ &)  = default;  \
    c_(c_ &&)                   = default;  \
    c_ & operator=(c_ &&)       = default;  \
    __VA_ARGS__ ~c_()

#define ROCKET_MOVABLE_DESTRUCTOR(c_, ...)  \
    c_(const c_ &)              =  delete;  \
    c_ & operator=(const c_ &)  =  delete;  \
    c_(c_ &&)                   = default;  \
    c_ & operator=(c_ &&)       = default;  \
    __VA_ARGS__ ~c_()

#define ROCKET_NONCOPYABLE_DESTRUCTOR(c_, ...)  \
    c_(const c_ &)              = delete;  \
    c_ & operator=(const c_ &)  = delete;  \
    c_(c_ &&)                   = delete;  \
    c_ & operator=(c_ &&)       = delete;  \
    __VA_ARGS__ ~c_()

template<typename typeT, typename withT>
  inline typeT exchange(typeT &ref, withT &&with)
  {
    auto old = ::std::move(ref);
    ref = ::std::forward<withT>(with);
    return old;
  }

    namespace details_utilities {

    using ::std::swap;

    template<typename typeT>
      struct is_nothrow_swappable : integral_constant<bool, noexcept(swap(::std::declval<typeT &>(), ::std::declval<typeT &>()))>
      {
      };

    template<typename typeT>
      void adl_swap(typeT &lhs, typeT &rhs) noexcept(is_nothrow_swappable<typeT>::value)
      {
        swap(lhs, rhs);
      }

    }

  using details_utilities::is_nothrow_swappable;
  using details_utilities::adl_swap;

template<typename lhsT, typename rhsT>
  constexpr typename common_type<lhsT &&, rhsT &&>::type min(lhsT &&lhs, rhsT &&rhs)
  {
    return (rhs < lhs) ? ::std::forward<rhsT>(rhs) : ::std::forward<lhsT>(lhs);
  }

template<typename lhsT, typename rhsT>
  constexpr typename common_type<lhsT &&, rhsT &&>::type max(lhsT &&lhs, rhsT &&rhs)
  {
    return (lhs < rhs) ? ::std::forward<rhsT>(rhs) : ::std::forward<lhsT>(lhs);
  }

template<typename charT, typename traitsT>
  inline void handle_ios_exception(basic_ios<charT, traitsT> &ios, typename basic_ios<charT, traitsT>::iostate &state)
  {
    // Set `ios_base::badbit` without causing `ios_base::failure` to be thrown.
    // Catch-then-ignore is **very** inefficient notwithstanding, it cannot be made more portable.
    try {
      ios.setstate(ios_base::badbit);
    } catch(...) {
      // Ignore this exception.
    }
    // Rethrow the **original** exception, if `ios_base::badbit` has been turned on in `os.exceptions()`.
    if(ios.exceptions() & ios_base::badbit) {
      throw;
    }
    // Clear the badbit as it need not be set a second time.
    state &= ~ios_base::badbit;
  }

template<typename iteratorT, typename functionT, typename ...paramsT>
  inline void ranged_for(iteratorT first, iteratorT last, functionT &&func, const paramsT &...params)
  {
    for(auto it = ::std::move(first); it != last; ++it) {
      ::std::forward<functionT>(func)(it, params...);
    }
  }

template<typename iteratorT, typename functionT, typename ...paramsT>
  inline void ranged_do_while(iteratorT first, iteratorT last, functionT &&func, const paramsT &...params)
  {
    auto it = ::std::move(first);
    do {
      ::std::forward<functionT>(func)(it, params...);
    } while(++it != last);
  }

template<typename ...unusedT>
  struct make_void
  {
    using type = void;
  };

template<typename ...typesT>
  struct conjunction : true_type
  {
  };
template<typename firstT, typename ...restT>
  struct conjunction<firstT, restT...> : conditional<(firstT::value ? false : true), firstT, conjunction<restT...>>::type
  {
  };

template<typename ...typesT>
  struct disjunction : false_type
  {
  };
template<typename firstT, typename ...restT>
  struct disjunction<firstT, restT...> : conditional<(firstT::value ? true : false), firstT, disjunction<restT...>>::type
  {
  };

    namespace details_utilities {

    template<typename iteratorT>
      constexpr size_t estimate_distance(::std::input_iterator_tag, iteratorT /*first*/, iteratorT /*last*/)
      {
        return 0;
      }
    template<typename iteratorT>
      inline size_t estimate_distance(::std::forward_iterator_tag, iteratorT first, iteratorT last)
      {
        size_t total = 0;
        for(auto it = ::std::move(first); it != last; ++it) {
          ++total;
        }
        return total;
      }
    template<typename iteratorT>
      constexpr size_t estimate_distance(::std::random_access_iterator_tag, iteratorT first, iteratorT last)
      {
        return static_cast<size_t>(last - first);
      }

    }

template<typename iteratorT>
  constexpr size_t estimate_distance(iteratorT first, iteratorT last)
  {
    return details_utilities::estimate_distance(typename iterator_traits<iteratorT>::iterator_category(), ::std::move(first), ::std::move(last));
  }

template<typename elementT, typename ...paramsT>
  elementT * construct_at(elementT *ptr, paramsT &&...params) noexcept(is_nothrow_constructible<elementT, paramsT &&...>::value)
  {
    ROCKET_ASSERT(ptr);
    return ::new(static_cast<void *>(ptr)) elementT(::std::forward<paramsT>(params)...);
  }

template<typename elementT>
  elementT * default_construct_at(elementT *ptr) noexcept(is_nothrow_default_constructible<elementT>::value)
  {
    ROCKET_ASSERT(ptr);
    return ::new(static_cast<void *>(ptr)) elementT;
  }

template<typename elementT>
  void destroy_at(elementT *ptr) noexcept(is_nothrow_destructible<elementT>::value)
  {
    ROCKET_ASSERT(ptr);
    return ptr->~elementT();
  }

template<typename elementT>
  void rotate(elementT *ptr, size_t begin, size_t seek, size_t end)
  {
    auto bot = begin;
    auto brk = seek;
    //   |<- isl ->|<- isr ->|
    //   bot       brk       end
    // > 0 1 2 3 4 5 6 7 8 9 -
    auto isl = brk - bot;
    if(isl == 0) {
      return;
    }
    auto isr = end - brk;
    if(isr == 0) {
      return;
    }
    auto stp = brk;
  r:
    if(isl < isr) {
      // Before:  bot   brk           end
      //        > 0 1 2 3 4 5 6 7 8 9 -
      // After:         bot   brk     end
      //        > 3 4 5 0 1 2 6 7 8 9 -
      do {
        noadl::adl_swap(ptr[bot++], ptr[brk++]);
      } while(bot != stp);
      // `isr` will have been decreased by `isl`, which will not result in zero.
      isr = end - brk;
      // `isl` is unchanged.
      stp = brk;
      goto r;
    }
    if(isl > isr) {
      // Before:  bot           brk   end
      //        > 0 1 2 3 4 5 6 7 8 9 -
      // After:       bot       brk   end
      //        > 7 8 9 3 4 5 6 0 1 2 -
      do {
        noadl::adl_swap(ptr[bot++], ptr[brk++]);
      } while(brk != end);
      // `isl` will have been decreased by `isr`, which will not result in zero.
      isl = stp - bot;
      // `isr` is unchanged.
      brk = stp;
      goto r;
    }
    // Before:  bot       brk       end
    //        > 0 1 2 3 4 5 6 7 8 9 -
    // After:             bot       brk
    //        > 5 6 7 8 9 0 1 2 3 4 -
    do {
      noadl::adl_swap(ptr[bot++], ptr[brk++]);
    } while(bot != stp);
  }

template<typename elementT>
  inline bool is_any_of(const elementT &elem, initializer_list<elementT> init)
  {
    auto ptr = init.begin();
    do {
      if(*ptr == elem) {
        return true;
      }
      if(++ptr == init.end()) {
        return false;
      }
    } while(true);
  }

template<typename elementT>
  inline bool is_none_of(const elementT &elem, initializer_list<elementT> init)
  {
    return !is_any_of(elem, init);
  }

template<typename enumT>
  constexpr typename underlying_type<enumT>::type weaken_enum(enumT value) noexcept
  {
    return static_cast<typename underlying_type<enumT>::type>(value);
  }

template<typename elementT, size_t countT>
  constexpr size_t countof(const elementT (&)[countT]) noexcept
  {
    return countT;
  }

}

#endif
