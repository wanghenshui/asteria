// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "compiler/simple_source_file.hpp"
#include "runtime/global_context.hpp"
#include "runtime/exception.hpp"
#include <iostream>

using namespace Asteria;

int main(int argc, char **argv)
  try {
    rocket::cow_vector<Reference> args;
    for(int i = 0; i < argc; ++i) {
      D_string arg;
      if(argv[i]) {
        arg += argv[i];
      }
      Reference_Root::S_constant ref_c = { std::move(arg) };
      args.emplace_back(std::move(ref_c));
    }
    Global_Context global;
    Simple_Source_File code(std::cin, std::ref("<stdin>"));
    auto res = code.execute(global, std::move(args));
    std::cout << res.read() << std::endl;
  } catch(Exception &e) {
    std::cerr << "Caught `Exception`: " << e.get_value() << std::endl;
  } catch(std::exception &e) {
    std::cerr << "Caught `std::exception`: " << e.what() << std::endl;
  }
