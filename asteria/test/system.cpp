// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      ::rocket::sref(__FILE__), __LINE__, ::rocket::sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        assert std.system.proc_invoke('true') == 0;
        assert std.system.proc_invoke('false') != 0;

        try {
          // note this may or may not throw
          var status = std.system.proc_invoke('./nonexistent-command');
          assert status != 0;
        }
        catch(e)
          assert std.string.find(e, "Assertion failure") == null;

        assert std.system.proc_invoke('bash',
          [ '-c', 'kill -1 $$' ]) == 129;
        assert std.system.proc_invoke('bash',
          [ '-c', 'kill -9 $$' ]) == 137;

        assert std.system.proc_invoke('bash',
          [ '-c', 'test $VAR == yes' ], [ 'VAR=yes' ]) == 0;
        assert std.system.proc_invoke('bash',
          [ '-c', 'test $VAR == yes' ], [ 'VAR=no' ]) != 0;

        var o = std.system.conf_load_file('test/sample.conf');
        assert o.key == "value";
        assert o["keys may be quoted"] == "equals signs are allowed";
        assert o.values.may == "be";
        assert o.values.objects == 42;
        assert o.and == ["can","also","be","arrays"];
        assert o.or == null;
        assert o.long_integers == 0x123456789ABCDEF;
        assert o.binary_integers == 0b10010010;
        assert __isinf o.infinities;
        assert __isnan o.nans;
        assert o.hexadecimal_float == 0x1.23p-62;
        assert o.binary_float == 0b100.0110p3;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    code.execute(global);
  }
