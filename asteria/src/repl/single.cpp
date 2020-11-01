// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "fwd.hpp"
#include "../runtime/global_context.hpp"
#include "../simple_script.hpp"
#include "../value.hpp"

namespace asteria {

void
load_and_execute_single_noreturn()
  try {
    // Prepare the parser.
    repl_script.set_options(repl_cmdline.opts);

    // Load and parse the script.
    try {
      if(repl_cmdline.path == "-")
        repl_script.reload_stdin();
      else
        repl_script.reload_file(repl_cmdline.path.c_str());
    }
    catch(exception& stdex) {
      printf_and_exit(exit_parser_error, "! error: %s\n", stdex.what());
    }

    // Execute the script, passing all command-line arguments to it.
    auto ref = repl_script.execute(repl_global,
                   cow_vector<Value>(repl_cmdline.args.begin(),
                                     repl_cmdline.args.end()));

    // If the script exits without returning a value, success is assumed.
    if(ref.is_void())
      printf_and_exit(exit_success);

    // Check whether the result is an integer.
    const auto& val = ref.dereference_readonly();
    if(!val.is_integer())
      printf_and_exit(exit_non_integer);

    // Exit with this code.
    printf_and_exit(static_cast<Exit_Status>(val.as_integer()));
  }
  catch(exception& stdex) {
    printf_and_exit(exit_runtime_error, "! error: %s\n", stdex.what());
  }

}  // namespace asteria
