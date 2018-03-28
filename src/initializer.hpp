// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Initializer {
public:
	enum Type : int {
		type_bracketed_init_list  = 0,
		type_braced_init_list     = 1,
		type_assignment           = 2,
	};
	struct Bracketed_init_list {
		Value_ptr_vector<Initializer> initializers;
	};
	struct Braced_init_list {
		Value_ptr_map<std::string, Initializer> key_values;
	};
	struct Assignment {
		Value_ptr<Expression> expression;
	};
	using Types = Type_tuple< Bracketed_init_list  // 0
	                        , Braced_init_list     // 1
	                        , Assignment           // 2
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Initializer, ValueT)>
	Initializer(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Initializer();

	ASTERIA_FORBID_COPY(Initializer)
	ASTERIA_ALLOW_MOVE(Initializer)

public:
	Value_ptr<Variable> evaluate_opt(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const;
};

}

#endif
