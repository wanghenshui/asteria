// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../llds/avm_rod.hpp"
#include "../llds/reference_stack.hpp"
#include "../llds/variable_hashmap.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference::
do_throw_not_dereferenceable() const
  {
    throw Runtime_Error(Runtime_Error::M_format(),
             "Reference type `$1` not dereferenceable",
             describe_xref(this->m_xref));
  }

void
Reference::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    this->m_value.collect_variables(staged, temp);

    if(auto var = unerase_pointer_cast<Variable>(this->m_var))
      if(staged.insert(&(this->m_var), var))
        temp.insert(var.get(), var);
  }

const Value&
Reference::
do_dereference_readonly_slow() const
  {
    const Value* valp = nullptr;
    size_t mi = 0;

    switch(this->m_xref) {
      case xref_invalid:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Reference not initialized");

      case xref_void:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Void reference not dereferenceable");

      case xref_temporary:
        valp = &(this->m_value);
        break;

      case xref_variable: {
        auto var = unerase_cast<Variable*>(this->m_var.get());
        ROCKET_ASSERT(var);

        if(!var->is_initialized())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Reference not initialized");

        valp = &(var->get_value());
        break;
      }

      case xref_ptc:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Proper tail call not expanded");

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_xref);
    }

    while(valp && (mi != this->m_mods.size()))
      valp = this->m_mods[mi++].apply_read_opt(*valp);

    if(!valp)
      return null_value;

    return *valp;
  }

Value&
Reference::
dereference_mutable() const
  {
    Value* valp = nullptr;
    size_t mi = 0;

    switch(this->m_xref) {
      case xref_invalid:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Reference not initialized");

      case xref_void:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Void reference not dereferenceable");

      case xref_temporary:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Attempt to modify a temporary value");

      case xref_variable: {
        auto var = unerase_cast<Variable*>(this->m_var.get());
        ROCKET_ASSERT(var);

        if(!var->is_initialized())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Reference not initialized");

        if(var->is_immutable())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Attempt to modify a `const` variable");

        valp = &(var->mut_value());
        break;
      }

      case xref_ptc:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Proper tail call not expanded");

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_xref);
    }

    while(valp && (mi != this->m_mods.size()))
      valp = &(this->m_mods[mi++].apply_open(*valp));

    return *valp;
  }

Value
Reference::
dereference_unset() const
  {
    Value* valp = nullptr;
    size_t mi = 0;

    switch(this->m_xref) {
      case xref_invalid:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Reference not initialized");

      case xref_void:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Void reference not dereferenceable");

      case xref_temporary:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Attempt to modify a temporary value");

      case xref_variable: {
        auto var = unerase_cast<Variable*>(this->m_var.get());
        ROCKET_ASSERT(var);

        if(!var->is_initialized())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Reference not initialized");

        if(var->is_immutable())
          throw Runtime_Error(Runtime_Error::M_format(),
                   "Attempt to modify a `const` variable");

        valp = &(var->mut_value());
        break;
      }

      case xref_ptc:
        throw Runtime_Error(Runtime_Error::M_format(),
                 "Proper tail call not expanded");

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_xref);
    }

    if(this->m_mods.size() == 0)
      throw Runtime_Error(Runtime_Error::M_format(),
               "Only elements of an array or object may be unset");

    while(valp && (mi != this->m_mods.size() - 1))
      valp = this->m_mods[mi++].apply_write_opt(*valp);

    if(!valp)
      return null_value;

    return this->m_mods[mi].apply_unset(*valp);
  }

void
Reference::
do_use_function_result_slow(Global_Context& global)
  {
    refcnt_ptr<PTC_Arguments> ptcg;
    cow_vector<refcnt_ptr<PTC_Arguments>> frames;
    opt<Value> result_value;
    Reference_Stack defer_stack, defer_alt_stack;
    Executive_Context defer_ctx(Executive_Context::M_defer(), global, defer_stack, defer_alt_stack);

    try {
      // Unpack frames until a non-PTC result is encountered.
      while(this->m_xref == xref_ptc) {
        ptcg.reset(unerase_cast<PTC_Arguments*>(this->m_ptc.release()));
        ROCKET_ASSERT(ptcg.use_count() == 1);
        ASTERIA_CALL_GLOBAL_HOOK(global, on_function_call, ptcg->sloc(), ptcg->target());

        // Perform a non-tail call.
        frames.emplace_back(ptcg);
        *this = ::std::move(ptcg->self());
        ptcg->target().invoke_ptc_aware(*this, global, ::std::move(ptcg->mut_stack()));
      }

      // Check the result.
      if(ptcg->ptc_aware() == ptc_aware_void)
        this->m_xref = xref_void;
      else if(this->m_xref != xref_void)
        result_value = this->dereference_readonly();

      // This is the normal return path.
      while(!frames.empty()) {
        ptcg = ::std::move(frames.mut_back());
        frames.pop_back();
        const auto caller = ptcg->caller_opt();

        // Convert the result.
        if((ptcg->ptc_aware() == ptc_aware_by_val) && result_value) {
          this->m_value = ::std::move(*result_value);
          result_value.reset();
          this->m_mods.clear();
          this->m_xref = xref_temporary;
        }

        // Evaluate deferred expressions.
        defer_ctx.stack() = ::std::move(ptcg->mut_stack());
        defer_ctx.mut_defer() = ::std::move(ptcg->mut_defer());
        defer_ctx.on_scope_exit_normal(air_status_next);

        // Leave this frame.
        if(caller)
          ASTERIA_CALL_GLOBAL_HOOK(global, on_function_return, defer_ctx, *caller,
                                   caller->zvarg()->sloc(), *this);
      }
    }
    catch(Runtime_Error& except) {
      // This is the exceptional path.
      while(!frames.empty()) {
        ptcg = ::std::move(frames.mut_back());
        frames.pop_back();
        const auto caller = ptcg->caller_opt();

        // Note that if we arrive here, there must have been an exception thrown
        // when unpacking the last frame (i.e. the last call did not return), so
        // the last frame does not have its enclosing function set.
        except.push_frame_plain(ptcg->sloc(), sref("[proper tail call]"));

        if(caller)
          except.push_frame_function(caller->zvarg()->sloc(), caller->zvarg()->func());

        // Evaluate deferred expressions.
        defer_ctx.stack() = ::std::move(ptcg->mut_stack());
        defer_ctx.mut_defer() = ::std::move(ptcg->mut_defer());
        defer_ctx.on_scope_exit_exceptional(except);

        // Leave this frame.
        if(caller)
          ASTERIA_CALL_GLOBAL_HOOK(global, on_function_except, defer_ctx, *caller,
                                   caller->zvarg()->sloc(), except);
      }

      // The exception object has been updated, so rethrow it.
      throw;
    }
  }

}  // namespace asteria
