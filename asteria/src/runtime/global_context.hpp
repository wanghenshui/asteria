// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_
#define ASTERIA_RUNTIME_GLOBAL_CONTEXT_HPP_

#include "../fwd.hpp"
#include "abstract_context.hpp"
#include "generational_collector.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  private:
    Generational_collector m_gen_coll;

  public:
    Global_context()
      {
        this->do_initialize_library();
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Global_context);

  private:
    void do_initialize_library();

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;

    Generational_collector & get_generational_collector() noexcept
      {
        return this->m_gen_coll;
      }
  };

}

#endif
