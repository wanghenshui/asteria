// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_OPAQUE_HPP_

#include "../fwd.hpp"
#include "refcnt_base.hpp"

namespace Asteria {

class Abstract_Opaque : public virtual RefCnt_Base
  {
  public:
    Abstract_Opaque() noexcept
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Abstract_Opaque, virtual);

  public:
    virtual void describe(std::ostream &os) const = 0;
    virtual Abstract_Opaque * clone(rocket::refcnt_object<Abstract_Opaque> &opaque_out) const = 0;
    virtual void enumerate_variables(const Abstract_Variable_Callback &callback) const = 0;
  };

inline std::ostream & operator<<(std::ostream &os, const Abstract_Opaque &opaque)
  {
    opaque.describe(os);
    return os;
  }

}

#endif
