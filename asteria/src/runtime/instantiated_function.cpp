// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "instantiated_function.hpp"
#include "reference.hpp"
#include "executive_context.hpp"
#include "statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

Instantiated_function::~Instantiated_function()
  {
  }

rocket::cow_string Instantiated_function::describe() const
  {
    Formatter fmt;
    ASTERIA_FORMAT(fmt, "function `", this->m_name, "(");
    auto param_it = this->m_params.begin();
    if(param_it != this->m_params.end()) {
      ASTERIA_FORMAT(fmt, *param_it);
      while(++param_it != this->m_params.end()) {
        ASTERIA_FORMAT(fmt, ", ", *param_it);
      }
    }
    ASTERIA_FORMAT(fmt, ")` at \'", this->m_loc, "\'");
    return fmt.extract_string();
  }

void Instantiated_function::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    this->m_body_bnd.enumerate_variables(callback);
  }

void Instantiated_function::invoke(Reference &result_out, Global_context &global, Reference &&self, rocket::cow_vector<Reference> &&args) const
  {
    this->m_body_bnd.execute_as_function(result_out, global, this->m_loc, this->m_name, this->m_params, &(this->m_zvarg), std::move(self), std::move(args));
  }

}
