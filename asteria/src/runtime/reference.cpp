// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference.hpp"
#include "global_context.hpp"
#include "abstract_hooks.hpp"
#include "evaluation_stack.hpp"
#include "executive_context.hpp"
#include "runtime_error.hpp"
#include "variable.hpp"
#include "ptc_arguments.hpp"
#include "enums.hpp"
#include "../util.hpp"

namespace asteria {
namespace {

Reference&
do_unpack_tail_calls(Reference& self, Global_Context& global)
  {
    // The function call shall yield an rvalue unless all wrapped calls return by reference.
    PTC_Aware ptc_conj = ptc_aware_by_ref;
    rcptr<PTC_Arguments> ptca;

    // We must rebuild the backtrace using this queue if an exception is thrown.
    cow_vector<rcptr<PTC_Arguments>> frames;
    Evaluation_Stack stack;

    ASTERIA_RUNTIME_TRY {
      // Unpack all frames recursively.
      // Note that `self` is overwritten before the wrapped function is called.
      while(!!(ptca = self.get_tail_call_opt())) {
        // Generate a single-step trap before unpacking arguments.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_single_step_trap(ptca->sloc());

        // Get the `this` reference and all the other arguments.
        auto args = ::std::move(ptca->open_arguments_and_self());
        self = ::std::move(args.mut_back());
        args.pop_back();

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_call(ptca->sloc(), ptca->get_target());

        // Figure out how to forward the result.
        if(ptca->ptc_aware() == ptc_aware_void) {
          ptc_conj = ptc_aware_void;
        }
        else if((ptca->ptc_aware() == ptc_aware_by_val) && (ptc_conj == ptc_aware_by_ref)) {
          ptc_conj = ptc_aware_by_val;
        }
        // Record this frame.
        frames.emplace_back(ptca);

        // Perform a non-tail call.
        ptca->get_target().invoke_ptc_aware(self, global, ::std::move(args));
      }

      // Check for deferred expressions.
      while(frames.size()) {
        // Pop frames in reverse order.
        ptca = ::std::move(frames.mut_back());
        frames.pop_back();

        // Evaluate deferred expressions if any.
        if(ptca->get_defer_stack().size())
          Executive_Context(::rocket::ref(global), ::rocket::ref(stack),
                            ::std::move(ptca->open_defer_stack()))
            .on_scope_exit(air_status_next);

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_return(ptca->sloc(), ptca->get_target(), self);
      }
    }
    ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
      // Check for deferred expressions.
      while(frames.size()) {
        // Pop frames in reverse order.
        ptca = ::std::move(frames.mut_back());
        frames.pop_back();

        // Push the function call.
        except.push_frame_plain(ptca->sloc(), ::rocket::sref("[proper tail call]"));

        // Call the hook function if any.
        if(auto qhooks = global.get_hooks_opt())
          qhooks->on_function_except(ptca->sloc(), ptca->get_target(), except);

        // Evaluate deferred expressions if any.
        if(ptca->get_defer_stack().size())
          Executive_Context(::rocket::ref(global), ::rocket::ref(stack),
                            ::std::move(ptca->open_defer_stack()))
            .on_scope_exit(except);

        // Push the caller.
        // Note that if we arrive here, there must have been an exception thrown when
        // unpacking the last frame (i.e. the last call did not return), so the last
        // frame does not have its enclosing function set.
        if(ptca->enclosing_sloc().offset() >= 0)
          except.push_frame_func(ptca->enclosing_sloc(), ptca->enclosing_func());
      }
      throw;
    }

    // Convert the result.
    if(ptc_conj == ptc_aware_void) {
      self = Reference_root::S_void();
    }
    else if((ptc_conj == ptc_aware_by_val) && self.is_glvalue()) {
      Reference_root::S_temporary xref = { self.read() };
      self = ::std::move(xref);
    }
    return self;
  }

}  // namespace

Reference::
~Reference()
  {
  }

void
Reference::
do_throw_unset_no_modifier()
const
  {
    ASTERIA_THROW("Non-members can't be unset");
  }

const Value&
Reference::
do_read(size_t nmod, const Reference_modifier& last)
const
  {
    auto qref = ::std::addressof(this->m_root.dereference_const());

    // Apply the first `nmod` modifiers.
    for(size_t i = 0;  i < nmod;  ++i)
      if(!(qref = this->m_mods[i].apply_const_opt(*qref)))
        return null_value;

    // Apply the last modifier.
    if(!(qref = last.apply_const_opt(*qref)))
      return null_value;
    return *qref;
  }

Value&
Reference::
do_open(size_t nmod, const Reference_modifier& last)
const
  {
    auto rref = ::rocket::ref(this->m_root.dereference_mutable());

    // Apply the first `nmod` modifiers.
    for(size_t i = 0;  i < nmod;  ++i)
      rref = ::rocket::ref(this->m_mods[i].apply_and_create(rref));

    // Apply the last modifier.
    rref = ::rocket::ref(last.apply_and_create(rref));
    return rref;
  }

Value
Reference::
do_unset(size_t nmod, const Reference_modifier& last)
const
  {
    auto qref = ::std::addressof(this->m_root.dereference_mutable());

    // Apply the first `nmod` modifiers.
    for(size_t i = 0;  i < nmod;  ++i)
      if(!(qref = this->m_mods[i].apply_mutable_opt(*qref)))
        return V_null();

    // Apply the last modifier.
    auto val = last.apply_and_erase(*qref);
    return val;
  }

Reference&
Reference::
do_finish_call(Global_Context& global)
  {
    return do_unpack_tail_calls(*this, global);
  }

Variable_Callback&
Reference::
enumerate_variables(Variable_Callback& callback)
const
  {
    return this->m_root.enumerate_variables(callback);
  }

}  // namespace asteria
