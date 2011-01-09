#include <stdio.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <vector>
#include <set>
#include <map>
#include <cctype>
#include <cstdarg>
#include <boost/function.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>

namespace jest {
	class fatal_error {};
}

namespace jest {namespace values {
	using namespace boost;
	using namespace std;

	namespace types
	{
		shared_ptr<void const> const type(new int);
	}

	namespace types
	{
		shared_ptr<void const> const symbol(new int);
		shared_ptr<void const> get_type_object_dispatch(string*)
		{return symbol;}
	}

	struct typed_value
	{
		typed_value(shared_ptr<void const> type,
				shared_ptr<void const> value): type(type), value(value) {}
		shared_ptr<void const> type;
		shared_ptr<void const> value;
	};

	namespace types
	{
		shared_ptr<void const> const typed_value(new int);
		shared_ptr<void const> get_type_object_dispatch(values::typed_value*)
		{return typed_value;}
	}

	struct typed_cell
	{
		typed_cell(shared_ptr<typed_value const> head,
				shared_ptr<typed_cell const> tail):
			head(head), tail(tail) {}
		shared_ptr<typed_value const> head;
		shared_ptr<typed_cell const> tail;
	};

	namespace types
	{
		shared_ptr<void const> const typed_cell(new int);
		shared_ptr<void const> get_type_object_dispatch(values::typed_cell*)
		{return typed_cell;}
	}

	struct pattern
	{
		pattern(shared_ptr<void const> const& type,
				shared_ptr<void const> const& value)
			: type(type), value(value) {}
		shared_ptr<void const> type;
		shared_ptr<void const> value;
	};

	namespace pattern_types
	{
		shared_ptr<void const> const constant(new int);
		shared_ptr<void const> const variable(new int);
	}

	struct pattern_cell
	{
		pattern_cell(shared_ptr<pattern const> const& head,
				shared_ptr<pattern const> const& tail)
			: head(head), tail(tail) {}
		shared_ptr<pattern const> head;
		shared_ptr<pattern const> tail;
	};

	namespace pattern_types
	{
		shared_ptr<void const> const cell(new int);
	}

	struct pattern_value
	{
		pattern_value(shared_ptr<pattern const> const& type,
				shared_ptr<pattern const> const& value):
			type(type), value(value) {}
		shared_ptr<pattern const> type;
		shared_ptr<pattern const> value;
	};

	namespace pattern_types
	{
		shared_ptr<void const> const value(new int);
	}

	struct binding
	{
		binding(shared_ptr<typed_value const> const& identifier,
				shared_ptr<typed_value const> const& value,
				shared_ptr<binding const> const& tail):
			identifier(identifier), value(value), tail(tail)  {}
		shared_ptr<typed_value const> identifier;
		shared_ptr<typed_value const> value;
		shared_ptr<binding const> tail;
	};

	struct rule
	{
		rule(
				shared_ptr<values::pattern const> const& pattern,
				shared_ptr<typed_value const> const& expression,
				shared_ptr<binding const> const& scope,
				shared_ptr<rule const> const& tail):
			pattern(pattern), expression(expression),
			scope(scope), tail(tail) {}

		shared_ptr<values::pattern const> pattern;
		shared_ptr<typed_value const> expression;
		shared_ptr<binding const> scope;
		shared_ptr<rule const> tail;
	};

	enum scoping_policy
	{
		scoping_policy_dynamic,
		scoping_policy_static
	};

	enum evaluation_policy
	{
		evaluation_policy_no_evaluate,
		evaluation_policy_evaluate
	};

	struct operator_
	{
		operator_(values::scoping_policy scoping_policy,
				values::evaluation_policy evaluation_policy,
				shared_ptr<rule const> const& rules):
		scoping_policy(scoping_policy),
		evaluation_policy(evaluation_policy),
		rules(rules) {}

		values::scoping_policy scoping_policy;
		values::evaluation_policy evaluation_policy;
		shared_ptr<rule const> rules;
	};

	namespace types
	{
		shared_ptr<void const> const operator_(new int);
		shared_ptr<void const> get_type_object_dispatch(values::operator_*)
		{return operator_;}
	}

	template <typename T> shared_ptr<void const> get_type_object()
	{
		return types::get_type_object_dispatch(static_cast<T*>(0));
	}

	template <typename T> shared_ptr<typed_value const> value(
			shared_ptr<T const> const& cell)
	{
		return make_shared<typed_value const>(get_type_object<T>(), cell);
	}
}}

namespace jest {namespace pattern_primitives {
	using namespace boost;
	using namespace values;

	shared_ptr<pattern const> pattern_nil()
	{
		return make_shared<pattern>(
				pattern_types::cell, shared_ptr<pattern_cell>());
	}

	shared_ptr<pattern const> pattern_cons(
			shared_ptr<pattern const> const& head,
			shared_ptr<pattern const> const& tail)
	{
		return make_shared<pattern>(pattern_types::cell,
				make_shared<pattern_cell>(head, tail));
	}
}}

namespace jest {namespace primitives {
	using namespace std;
	using namespace boost;
	using namespace values;

	template <typename T> shared_ptr<T const> downcast(
			shared_ptr<typed_value const> const& value)
	{
		assert(value->type == get_type_object<T>());
		return static_pointer_cast<T const>(value->value);
	}

	struct symbol_less : public less<shared_ptr<typed_value const> >
	{
		bool operator()(shared_ptr<typed_value const> const& x0,
				shared_ptr<typed_value const> const& x1) const
		{
			return *static_pointer_cast<string const>(x0->value) <
				*static_pointer_cast<string const>(x1->value);
		}
	};

	shared_ptr<typed_value const> symbol(string const& text)
	{
		static set<shared_ptr<typed_value const>, symbol_less> symbols;
		return *symbols.insert(make_shared<typed_value>(
						types::symbol,
						make_shared<string>(text))).first;
	}

	shared_ptr<typed_value const> builtin_symbol(string const& text)
	{
		static set<shared_ptr<typed_value const>, symbol_less> symbols;
		return *symbols.insert(make_shared<typed_value>(
						types::symbol,
						make_shared<string>(text))).first;
	}

	shared_ptr<typed_cell const> cons(
			shared_ptr<typed_value const> const& head,
			shared_ptr<typed_cell const> const& tail)
	{
		shared_ptr<typed_cell const> cell(new typed_cell(head, tail));
		return cell;
	}

	int length(shared_ptr<typed_cell const> const& l)
	{
		int n = 0;
		for (shared_ptr<typed_cell const>  p = l; p; p = p->tail)
			++n;
		return n;
	}

	shared_ptr<typed_value const> car(shared_ptr<typed_cell const> const&
			list)
	{
		return list->head;
	}

	shared_ptr<typed_value const> car(shared_ptr<typed_value const> const&
			list)
	{
		assert(list->type == types::typed_cell);
		return car(static_pointer_cast<typed_cell const>(list->value));
	}

	shared_ptr<typed_cell const> cdr(shared_ptr<typed_cell const> const&
			list)
	{
		return list->tail;
	}

	shared_ptr<typed_cell const> cdr(shared_ptr<typed_value const> const&
			list)
	{
		assert(list->type == types::typed_cell);
		return cdr(static_pointer_cast<typed_cell const>(list->value));
	}

	shared_ptr<typed_value const> cadr(shared_ptr<typed_cell const> const&
			list)
	{
		return list->tail->head;
	}

	shared_ptr<typed_value const> cadr(shared_ptr<typed_value const> const&
			list)
	{
		assert(list->type == types::typed_cell);
		return cadr(static_pointer_cast<typed_cell const>(list->value));
	}

	shared_ptr<typed_cell const> cddr(shared_ptr<typed_cell const> const&
			list)
	{
		return list->tail->tail;
	}

	shared_ptr<typed_cell const> cddr(shared_ptr<typed_value const> const&
			list)
	{
		assert(list->type == types::typed_cell);
		return cddr(static_pointer_cast<typed_cell const>(list->value));
	}

	shared_ptr<string const> as_symbol(shared_ptr<typed_value const> const&
			value)
	{
		assert(value->type == types::symbol);
		return static_pointer_cast<string const>(value->value);
	}

	shared_ptr<typed_cell const> nil()
	{
		return shared_ptr<typed_cell const>();
	}

	shared_ptr<typed_cell const> list(
			shared_ptr<typed_value const> const& x0)
	{
		return cons(x0, nil());
	}

	shared_ptr<typed_cell const> list(
			shared_ptr<typed_value const> const& x0,
			shared_ptr<typed_value const> const& x1)
	{
		return cons(x0, list(x1));
	}

	shared_ptr<typed_cell const> list(
			shared_ptr<typed_value const> const& x0,
			shared_ptr<typed_value const> const& x1,
			shared_ptr<typed_value const> const& x2)
	{
		return cons(x0, list(x1, x2));
	}

	shared_ptr<typed_cell const> list(
			shared_ptr<typed_value const> const& x0,
			shared_ptr<typed_value const> const& x1,
			shared_ptr<typed_value const> const& x2,
			shared_ptr<typed_value const> const& x3)
	{
		return cons(x0, list(x1, x2, x3));
	}

	shared_ptr<typed_value const> pop(shared_ptr<typed_cell const>& list)
	{
		shared_ptr<typed_value const> value = list->head;
		list = list->tail;
		return value;
	}
}}

namespace jest {namespace values {
	shared_ptr<typed_value const> const enclosing_scope(
			primitives::symbol("enclosing_scope"));
}}

namespace jest {namespace special_symbols {
	using namespace boost;
	using namespace values;

	namespace detail
	{
		shared_ptr<typed_value const> const symbol(std::string const& text)
		{
			return make_shared<typed_value>(
				typed_value(types::symbol, make_shared<string>(text)));
		}
	}

	shared_ptr<typed_value const> const quote = detail::symbol("quote");
	shared_ptr<typed_value const> const var = detail::symbol("var");
	shared_ptr<typed_value const> const rule = detail::symbol("rule");
	shared_ptr<typed_value const> const module = detail::symbol("module");
	shared_ptr<typed_value const> const pattern = detail::symbol("pattern");
	shared_ptr<typed_value const> const native = detail::symbol("native");
	shared_ptr<typed_value const> const template_ = detail::symbol("template");
	shared_ptr<typed_value const> const members = detail::symbol("members");
}}

namespace jest {namespace patterns {
	using namespace values;

	struct context
	{
	};

	void fatal(context* c, char const* format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		fputs("\n", stderr);
		throw fatal_error();
	}

	bool equal(context* c,
			shared_ptr<void const> const& type0,
			shared_ptr<void const> const& value0,
			shared_ptr<void const> const& type1,
			shared_ptr<void const> const& value1)
	{
		if (type0 != type1)
			return false;

		if (type0 == types::type)
		{
			// TODO: parameterized types.
			return value0 == value1;
		}
		else if (type0 == types::symbol)
		{
			return value0 == value1;
		}
		else if (type0 == types::typed_cell)
		{
			shared_ptr<typed_cell const> cell0 = static_pointer_cast<
				typed_cell const>(value0);
			shared_ptr<typed_cell const> cell1 = static_pointer_cast<
				typed_cell const>(value1);

			return
				equal(c,
						cell0->head->type, cell0->head->value,
						cell1->head->type, cell1->head->value) &&
				equal(c,
						types::typed_cell, cell0->tail,
						types::typed_cell, cell1->tail);
		}
		else if (type0 == types::typed_value)
		{
			shared_ptr<typed_value const> val0 = static_pointer_cast<
				typed_value const>(value0);
			shared_ptr<typed_value const> val1 = static_pointer_cast<
				typed_value const>(value1);

			return equal(c,
					val0->type, val0->value,
					val1->type, val1->type);
		}
		else
		{
			fatal(c, "equal() cannot compare values "
					"of unrecognized types.");
			return false;
		}
	}

	struct binding
	{
		binding(shared_ptr<typed_value const> symbol,
				shared_ptr<void const> type,
				shared_ptr<void const> value)
			: symbol(symbol), type(type), value(value) {}
		shared_ptr<typed_value const> symbol;
		shared_ptr<void const> type;
		shared_ptr<void const> value;
	};

	struct match_result
	{
		vector<shared_ptr<binding const> > bindings;
	};

	shared_ptr<match_result const> match(
			context* c,
			shared_ptr<void const> const& pattern_type,
			shared_ptr<void const> const& pattern_value,
			shared_ptr<void const> const& type,
			shared_ptr<void const> const& value)
	{
		if (pattern_type == pattern_types::constant)
		{
			shared_ptr<typed_value const> pattern_typed_value =
				static_pointer_cast<typed_value const>(
						pattern_value);
			if (!equal(c, pattern_typed_value->type,
						pattern_typed_value->value, type, value))
				return shared_ptr<match_result const>();
			shared_ptr<match_result> result(new match_result);
			return result;
		}
		else if (pattern_type == pattern_types::variable)
		{
			shared_ptr<typed_value const> pattern_symbol =
				static_pointer_cast<typed_value const>(pattern_value);
			shared_ptr<patterns::binding> binding(new patterns::binding(
						pattern_symbol, type, value));
			shared_ptr<match_result> result(new match_result);
			result->bindings.push_back(binding);
			return result;
		}
		else if (pattern_type == pattern_types::cell && !pattern_value)
		{
			// We need to match an empty list.
			if (type != types::typed_cell)
				return shared_ptr<match_result const>();
			return make_shared<match_result>();
		}
		else if (pattern_type == pattern_types::cell ||
				pattern_type == pattern_types::value)
		{
			shared_ptr<pattern const> pattern_head, pattern_tail;
			shared_ptr<void const> value_head_type, value_head_value;
			shared_ptr<void const> value_tail_type, value_tail_value;
			if (pattern_type == pattern_types::cell)
			{
				if (type != types::typed_cell)
					return shared_ptr<match_result const>();
				shared_ptr<values::pattern_cell const> pattern_cell =
					static_pointer_cast<values::pattern_cell const>(
							pattern_value);
				shared_ptr<typed_cell const> cell =
					static_pointer_cast<typed_cell const>(value);

				pattern_head = pattern_cell->head;
				pattern_tail = pattern_cell->tail;
				value_head_type = cell->head->type;
				value_head_value = cell->head->value;
				value_tail_type = types::typed_cell;
				value_tail_value = cell->tail;
			}
			else
			{
				shared_ptr<values::pattern_value const> pattern_typed_value =
					static_pointer_cast<values::pattern_value const>(
							pattern_value);

				pattern_head = pattern_typed_value->type;
				pattern_tail = pattern_typed_value->value;
				value_head_type = types::type;
				value_head_value = type;
				value_tail_type = type;
				value_tail_value = value;
			}

			shared_ptr<match_result const> head_result = match(c,
					pattern_head->type, pattern_head->value,
					value_head_type, value_head_value);
			if (!head_result)
				return head_result;
			shared_ptr<match_result const> tail_result = match(c,
					pattern_tail->type, pattern_tail->value,
					value_tail_type, value_tail_value);
			if (!tail_result)
				return tail_result;

			// Combine bindings.
			shared_ptr<match_result> result(new match_result);
			result->bindings = head_result->bindings;
			for (int j = 0, jcnt = int(tail_result->bindings.size());
					j < jcnt; ++j)
			{
				shared_ptr<binding const> tail_binding =
					tail_result->bindings[j];

				shared_ptr<binding const> matching_binding;
				for (int i = 0, icnt = int(head_result->bindings.size());
						i < icnt; ++i)
				{
					shared_ptr<binding const> head_binding =
						head_result->bindings[i];

					if (head_binding->symbol == tail_binding->symbol)
						matching_binding = head_binding;
				}

				if (matching_binding)
				{
					if (!equal(c, tail_binding->type, tail_binding->value,
								tail_binding->type, tail_binding->value))
						return shared_ptr<match_result const>();
				}
				else
				{
					result->bindings.push_back(tail_binding);
				}
			}

			return result;
		}
		else
		{
			fatal(c, "INTERNAL ERROR: Invalid pattern type.");
			return shared_ptr<match_result const>();
		}
	}
}}

#define parse_assert(c, cond) \
	do \
	{ \
		if (!cond) \
		{ \
			fatal((c), "internal parsing error: %s", #cond); \
		} \
	} while (0)

namespace jest {namespace parsing {

	using namespace boost;
	using namespace std;

	typedef std::string identifier;

	struct form;

	typedef variant<
		shared_ptr<parsing::identifier const>,
		shared_ptr<parsing::form const> > expression;

	struct parameter
	{
		parameter(shared_ptr<identifier const> const& name,
				shared_ptr<expression const> const& type)
			: name(name), type(type) {}

		shared_ptr<identifier const> name;
		shared_ptr<expression const> type;
	};

	struct prototype
	{
		prototype(shared_ptr<expression const> const& name,
				vector<shared_ptr<parameter const> > const& parameters)
			: name(name), parameters(parameters) {}
		shared_ptr<expression const> name;
		vector<shared_ptr<parameter const> > parameters;
	};

	typedef variant<
		shared_ptr<parsing::identifier const>,
		shared_ptr<parsing::prototype const> > target;

	struct define;

	typedef variant<
		shared_ptr<parsing::define const>,
		shared_ptr<parsing::expression const> > statement;

	struct form
	{
		form(shared_ptr<expression const> const& head,
				vector<shared_ptr<parsing::statement const> > const&
				statements)
			: head(head), statements(statements) {}
		shared_ptr<expression const> head;
		std::vector<shared_ptr<parsing::statement const> > statements;
	};

	struct define
	{
		define(shared_ptr<parsing::target const> const& target,
				shared_ptr<parsing::expression const> const& expression)
			: target(target), expression(expression) {}
		shared_ptr<parsing::target const> target;
		shared_ptr<parsing::expression const> expression;
	};

	struct module
	{
		module(std::vector<shared_ptr<parsing::define const> > const&
				defines)
			: defines(defines) {}
		std::vector<shared_ptr<parsing::define const> > defines;
	};

	enum symbol
	{
		eof,
		ident,
		lt,
		gt,
		colon,
		equal
	};
	struct context
	{
		FILE* f;
		parsing::symbol symbol;
		string token;
		char ch;
	};

	shared_ptr<module const> parse_module(context* c);
	shared_ptr<define const> parse_define(context* c);
	shared_ptr<statement const> parse_statement(context* c);
	shared_ptr<statement const> parse_symbol_statement(context* c);
	shared_ptr<define const> parse_symbol_define(context* c);
	shared_ptr<statement const> parse_form_statement(context* c);
	shared_ptr<statement const> parse_form_statement_contents(context* c);
	struct form_statement_tail
	{
		form_statement_tail(
				vector<shared_ptr<parameter const> > const& parameters,
				vector<shared_ptr<statement const> > const& statements,
				shared_ptr<parsing::expression const> const& expression):
			parameters(parameters),
			statements(statements),
			expression(expression)
		{}
		vector<shared_ptr<parameter const> > parameters;
		vector<shared_ptr<statement const> > statements;
		shared_ptr<parsing::expression const> expression;
	};
	shared_ptr<form_statement_tail const> parse_form_statement_tail(
			context* c);
	struct prototype_non_thunk_tail
	{
		prototype_non_thunk_tail(shared_ptr<parsing::expression const>
				const& initial_type,
				vector<shared_ptr<parameter const> > const&
				subsequent_parameters,
				shared_ptr<parsing::expression const> const& expression):
			initial_type(initial_type),
			subsequent_parameters(subsequent_parameters),
			expression(expression)
		{}
		shared_ptr<parsing::expression const> initial_type;
		vector<shared_ptr<parameter const> > subsequent_parameters;
		shared_ptr<parsing::expression const> expression;
	};
	struct form_expression_non_thunk_tail
	{
		form_expression_non_thunk_tail(
				vector<shared_ptr<statement const> > const&
				subsequent_arguments):
			subsequent_arguments(subsequent_arguments) {}
		vector<shared_ptr<statement const> > subsequent_arguments;
	};
	typedef variant<
		shared_ptr<prototype_non_thunk_tail const>,
		shared_ptr<form_expression_non_thunk_tail const> > non_thunk_tail;
	shared_ptr<non_thunk_tail const> parse_non_thunk_tail(context* c);
	shared_ptr<expression const> parse_define_tail(context* c);
	struct form_expression_tail
	{
		form_expression_tail(
				vector<shared_ptr<statement const> > const& statements)
			: statements(statements) {}
		vector<shared_ptr<statement const> > statements;
	};
	shared_ptr<form_expression_tail const> parse_form_expression_tail(
			context* c);
	shared_ptr<define const> parse_form_define(context* c);
	shared_ptr<parameter const> parse_parameter(context* c);
	shared_ptr<expression const> parse_expression(context* c);
	shared_ptr<expression const> parse_form_expression(context* c);

	void error(context* c, char const* format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		fputs("\n", stderr);
	}

	void fatal(context* c, char const* format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		fputs("\n", stderr);
		throw fatal_error();
	}

	void read_symbol(context* c)
	{
		while (std::isspace(c->ch))
			c->ch = fgetc(c->f);

		if (feof(c->f))
		{
			c->symbol = eof;
			c->token = "";
		}
		else if (c->ch == '<')
		{
			c->symbol = lt;
			c->token = "<";
			c->ch = fgetc(c->f);
		}
		else if (c->ch == '>')
		{
			c->symbol = gt;
			c->token = ">";
			c->ch = fgetc(c->f);
		}
		else if (c->ch == ':')
		{
			c->symbol = colon;
			c->token = ":";
			c->ch = fgetc(c->f);
		}
		else if (c->ch == '=')
		{
			c->symbol = equal;
			c->token = "=";
			c->ch = fgetc(c->f);
		}
		else if (std::isalnum(c->ch) || c->ch == '_')
		{
			c->symbol = ident;
			c->token = "";
			while (std::isalnum(c->ch) || c->ch == '_')
			{
				c->token.push_back(c->ch);
				c->ch = fgetc(c->f);
			}
		}
		else
		{
			fatal(c, "invalid character \'%c\'.", c->ch);
			c->ch = fgetc(c->f);
		}
	}

	bool accept(context* c, symbol symbol)
	{
		if (c->symbol == symbol)
		{
			read_symbol(c);
			return true;
		}
		return false;
	}

	void expect(context* c, symbol symbol)
	{
		if (c->symbol != symbol)
			fatal(c, "unexpected: %s", c->token.c_str());
		read_symbol(c);
	}

	shared_ptr<identifier const> parse_identifier(context* c)
	{
		if (c->symbol == ident)
		{
			shared_ptr<identifier> identifier(new string(c->token));
			read_symbol(c);
			return identifier;
		}
		return shared_ptr<identifier const>();
	}

	//module = {define}
	shared_ptr<module const> parse_module(context* c)
	{
		std::vector<shared_ptr<parsing::define const> > defines;
		while (c->symbol != eof)
		{
			shared_ptr<define const> define = parse_define(c);
			parse_assert(c, define);
			defines.push_back(define);
		}
		shared_ptr<module> module(new parsing::module(defines));

		return module;
	}

	//define =
	//	form_define
	//	| symbol_define
	shared_ptr<define const> parse_define(context* c)
	{
		shared_ptr<define const> define;
		define = (!define ? parse_form_define(c) : define);
		define = (!define ? parse_symbol_define(c) : define);
		return define;
	}

	//statement =
	//	form_statement
	//	| symbol_statement
	shared_ptr<statement const> parse_statement(context* c)
	{
		shared_ptr<statement const> statement;
		statement = (!statement ? parse_form_statement(c) : statement);
		statement = (!statement ? parse_symbol_statement(c) : statement);
		return statement;
	}

	//symbol_statement =
	//	identifier ["=" expression]
	shared_ptr<statement const> parse_symbol_statement(context* c)
	{
		shared_ptr<parsing::identifier const> identifier =
			parse_identifier(c);
		if (!identifier)
			return shared_ptr<parsing::statement const>();
		if (accept(c, equal))
		{
			shared_ptr<parsing::expression const> expression =
				parse_expression(c);
			shared_ptr<parsing::target> target(
					new parsing::target(identifier));
			shared_ptr<parsing::define> define(
					new parsing::define(
						target, expression));
			shared_ptr<parsing::statement> statement(
					new parsing::statement(define));
			return statement;
		}
		else
		{
			shared_ptr<parsing::expression> expression(
					new parsing::expression(identifier));
			shared_ptr<parsing::statement> statement(
					new parsing::statement(expression));
			return statement;
		}
	}

	//symbol_define =
	//	identifier "=" expression
	shared_ptr<define const> parse_symbol_define(context* c)
	{
		shared_ptr<identifier const> identifier = parse_identifier(c);
		if (!identifier)
			return shared_ptr<define const>();
		expect(c, equal);
		shared_ptr<expression const> expression = parse_expression(c);
		shared_ptr<parsing::target> target(
				new parsing::target(identifier));
		shared_ptr<parsing::define> define(
				new parsing::define(target, expression));
		return define;
	}

	//form_statement =
	//	"<" form_statement_contents
	shared_ptr<statement const> parse_form_statement(context* c)
	{
		if (!accept(c, lt))
			return shared_ptr<statement const>();
		return parse_form_statement_contents(c);
	}

	//form_statement_contents =
	//	identifier form_statement_tail
	//	| {statement} ">"
	shared_ptr<statement const> parse_form_statement_contents(context* c)
	{
		shared_ptr<identifier const> identifier = parse_identifier(c);
		if (identifier)
		{
			shared_ptr<form_statement_tail const> tail =
				parse_form_statement_tail(c);

			parse_assert(c, tail);
			if (!tail->parameters.empty())
			{
				parse_assert(c, tail->statements.empty());

				shared_ptr<parsing::expression> name(
						new parsing::expression(identifier));
				shared_ptr<parsing::prototype> prototype(
						new parsing::prototype(name, tail->parameters));
				shared_ptr<parsing::target> target(
						new parsing::target(prototype));
				shared_ptr<parsing::define> define(
						new parsing::define(target, tail->expression));
				shared_ptr<parsing::statement> statement(
						new parsing::statement(define));
				return statement;
			}
			else if (!tail->statements.empty())
			{
				shared_ptr<parsing::expression> head(
						new parsing::expression(identifier));
				shared_ptr<parsing::form> form(
						new parsing::form(head, tail->statements));
				shared_ptr<parsing::expression> expression(
						new parsing::expression(form));
				shared_ptr<parsing::statement> statement(
						new parsing::statement(expression));
				return statement;
			}
			else
			{
				parse_assert(c, 0);
				return shared_ptr<statement const>();
			}
		}
		else
		{
			shared_ptr<expression const> head = parse_expression(c);
			parse_assert(c, head);
			vector<shared_ptr<statement const> > statements;
			while (c->symbol != gt)
			{
				shared_ptr<statement const> statement =
					parse_statement(c);
				parse_assert(c, statement);
				statements.push_back(statement);
			}

			expect(c, gt);

			shared_ptr<parsing::form> form(
					new parsing::form(head, statements));
			shared_ptr<parsing::expression> expression(
					new parsing::expression(form));
			shared_ptr<parsing::statement> statement(
					new parsing::statement(expression));

			return statement;
		}
	}

	//form_statement_tail =
	//	">" [define_tail]
	//	| identifier non_thunk_tail
	//	| {statement} ">"
	shared_ptr<form_statement_tail const> parse_form_statement_tail(
			context* c)
	{
		vector<shared_ptr<parameter const> > parameters;
		vector<shared_ptr<statement const> > statements;
		shared_ptr<expression const> expression;

		if (c->symbol == gt)
		{
			expression = parse_define_tail(c);
		}
		else
		{
			shared_ptr<identifier const> identifier = parse_identifier(c);
			if (identifier)
			{
				shared_ptr<non_thunk_tail const> tail =
					parse_non_thunk_tail(c);
				shared_ptr<prototype_non_thunk_tail const> const* ptail =
					boost::get<
					shared_ptr<prototype_non_thunk_tail const> >(
							tail.get());
				if (ptail)
				{
					shared_ptr<parameter> initial_parameter(new parameter(
								identifier, (*ptail)->initial_type));
					parameters.push_back(initial_parameter);
					for (int i = 0, cnt =
							int((*ptail)->subsequent_parameters.size());
							i < cnt; ++i)
						parameters.push_back(
								(*ptail)->subsequent_parameters[i]);
					expression = (*ptail)->expression;
				}
				else if (shared_ptr<form_expression_non_thunk_tail const>
						const* ftail = boost::get<shared_ptr<
						form_expression_non_thunk_tail const> >(
							tail.get()))
				{
					shared_ptr<parsing::expression> expression(
							new parsing::expression(identifier));
					shared_ptr<parsing::statement> statement(
							new parsing::statement(expression));
					statements.push_back(statement);
					for (int i = 0,
							cnt = int(
								(*ftail)->subsequent_arguments.size());
							i < cnt; ++i)
						statements.push_back(
								(*ftail)->subsequent_arguments[i]);
				}
				else
				{
					parse_assert(c, 0);
				}
			}
			else
			{
				while (c->symbol != gt)
				{
					shared_ptr<statement const> statement =
						parse_statement(c);
					parse_assert(c, statement);
					statements.push_back(statement);
				}
				expect(c, gt);
			}
		}

		shared_ptr<form_statement_tail> tail(new form_statement_tail(
					parameters, statements, expression));
		return tail;
	}

	//non_thunk_tail =
	//	":" expression {parameter} ">" define_tail
	//	| {statement} ">"
	shared_ptr<non_thunk_tail const> parse_non_thunk_tail(context* c)
	{
		if (accept(c, colon))
		{
			shared_ptr<expression const> initial_type =
				parse_expression(c);
			vector<shared_ptr<parameter const> > subsequent_parameters;
			while (c->symbol != gt)
			{
				shared_ptr<parameter const> parameter =
					parse_parameter(c);
				parse_assert(c, parameter);
				subsequent_parameters.push_back(parameter);
			}
			expect(c, gt);
			shared_ptr<expression const> expression =
				parse_define_tail(c);
			shared_ptr<prototype_non_thunk_tail> ptail(
					new prototype_non_thunk_tail(
						initial_type, subsequent_parameters, expression));
			shared_ptr<non_thunk_tail> tail(new non_thunk_tail(ptail));
			return tail;
		}
		else
		{
			vector<shared_ptr<statement const> > subsequent_arguments;
			while (c->symbol != gt)
			{
				shared_ptr<statement const> statement =
					parse_statement(c);
				parse_assert(c, statement);
				subsequent_arguments.push_back(statement);
			}
			expect(c, gt);
			shared_ptr<form_expression_non_thunk_tail> ftail(new
					form_expression_non_thunk_tail(subsequent_arguments));
			shared_ptr<non_thunk_tail> tail(new non_thunk_tail(ftail));
			return tail;
		}
	}

	//define_tail =
	//	"=" expression
	shared_ptr<expression const> parse_define_tail(context* c)
	{
		if (!accept(c, equal))
			return shared_ptr<expression const>();
		shared_ptr<expression const> expression = parse_expression(c);
		parse_assert(c, expression);
		return expression;
	}

	//form_expression_tail =
	//	{statement} ">"
	shared_ptr<form_expression_tail const> parse_form_expression_tail(
			context* c)
	{
		vector<shared_ptr<statement const> > statements;
		while (c->symbol != gt)
		{
			shared_ptr<parsing::statement const> statement =
				parse_statement(c);
			parse_assert(c, statement);
			statements.push_back(statement);
		}
		expect(c, gt);
		shared_ptr<form_expression_tail> tail(
				new form_expression_tail(statements));
		return tail;
	}

	//form_define =
	//	"<" expression {parameter} ">" = expression
	shared_ptr<define const> parse_form_define(context* c)
	{
		if (!accept(c, lt))
			return shared_ptr<define const>();

		shared_ptr<expression const> name = parse_expression(c);
		parse_assert(c, name);
		vector<shared_ptr<parameter const> > parameters;
		while (c->symbol != gt)
		{
			shared_ptr<parameter const> parameter = parse_parameter(c);
			parse_assert(c, parameter);
			parameters.push_back(parameter);
		}
		shared_ptr<parsing::prototype> prototype(
				new parsing::prototype(name, parameters));

		expect(c, gt);
		expect(c, equal);

		shared_ptr<parsing::expression const> expression =
			parse_expression(c);
		shared_ptr<parsing::target> target(
				new parsing::target(prototype));
		shared_ptr<parsing::define> define(
				new parsing::define(target, expression));
		return define;
	}

	//parameter =
	//	identifier ":" identifier
	shared_ptr<parameter const> parse_parameter(context* c)
	{
		shared_ptr<identifier const> name = parse_identifier(c);
		parse_assert(c, name);

		expect(c, colon);

		shared_ptr<expression const> type = parse_expression(c);
		parse_assert(c, type);
		shared_ptr<parsing::parameter> parameter(
				new parsing::parameter(name, type));
		return parameter;
	}

	//expression =
	//	identifier
	//	| form_expression
	shared_ptr<expression const> parse_expression(context* c)
	{
		shared_ptr<parsing::identifier const> identifier =
			parse_identifier(c);
		if (identifier)
		{
			shared_ptr<parsing::expression> expression(
					new parsing::expression(identifier));
			return expression;
		}
		return parse_form_expression(c);
	}

	//form_expression =
	//	"<" expression form_expression_tail
	shared_ptr<expression const> parse_form_expression(context* c)
	{
		if (!accept(c, lt))
			return shared_ptr<expression const>();
		shared_ptr<expression const> head = parse_expression(c);
		parse_assert(c, head);
		shared_ptr<form_expression_tail const> tail =
			parse_form_expression_tail(c);
		parse_assert(c, tail);
		shared_ptr<parsing::form> form(new parsing::form(
					head, tail->statements));
		shared_ptr<parsing::expression> expression(
				new parsing::expression(form));
		return expression;
	}

	shared_ptr<parsing::module const> parse_file(char const* filename)
	{
		context c;
		c.f = fopen(filename, "r");
		if (!c.f)
		{
			char buf[1024];
			perror(buf);
			fatal(&c, "error opening file \"%s\": %s", filename, buf);
		}

		c.ch = fgetc(c.f);
		read_symbol(&c);

		return parse_module(&c);
	}
}}

namespace jest {namespace generation {

	using namespace boost;
	using namespace values;

	shared_ptr<typed_value const> generate_expression(
			shared_ptr<parsing::expression const> const& parse_expression);

	shared_ptr<typed_value const> generate_statement(
			shared_ptr<parsing::statement const> const& parse_statement);

	struct expression_generator :
		public static_visitor<shared_ptr<typed_value const> >
	{
		shared_ptr<typed_value const> operator()(
				shared_ptr<parsing::identifier const> const& identifier)
			const
		{
			using namespace primitives;
			return symbol(*identifier);
		}

		shared_ptr<typed_value const> operator()(
				shared_ptr<parsing::form const> const& form) const
		{
			using namespace primitives;

			shared_ptr<typed_cell const> args = nil();
			for (int i = int(form->statements.size()) - 1; i >= 0; --i)
				args = cons(generate_statement(form->statements[i]), args);

			return value(cons(generate_expression(form->head), args));
		}
	};

	shared_ptr<typed_value const> generate_expression(
			shared_ptr<parsing::expression const> const& parse_expression)
	{
		return apply_visitor(expression_generator(), *parse_expression);
	}

	shared_ptr<typed_value const> generate_pattern(
			vector<shared_ptr<parsing::parameter const> > const& parameters)
	{
		using namespace pattern_primitives;
		using namespace primitives;

		shared_ptr<typed_cell const> args = nil();
		for (int prm = int(parameters.size()) - 1; prm >= 0; --prm)
		{
			args = cons(
					value(list(
						symbol(*parameters[prm]->name),
						generate_expression(parameters[prm]->type))),
					args);
		}

		return value(cons(special_symbols::pattern, args));
	}

	struct define_generator :
		public static_visitor<shared_ptr<typed_value const> >
	{
		define_generator(
				shared_ptr<parsing::expression const> const& expression)
			: expression(expression) {}

		shared_ptr<parsing::expression const> expression;

		shared_ptr<typed_value const> operator()(
				shared_ptr<parsing::prototype const> const& target_prototype)
			const
		{
			using namespace primitives;

			return value(list(
					special_symbols::rule,
					generate_expression(target_prototype->name),
					generate_pattern(target_prototype->parameters),
					generate_expression(this->expression)));
		}

		shared_ptr<typed_value const> operator()(
				shared_ptr<parsing::identifier const> const& target_symbol)
			const
		{
			using namespace primitives;

			return value(list(
					special_symbols::var,
					symbol(*target_symbol),
					generate_expression(this->expression)));
		}
	};

	shared_ptr<typed_value const> generate_define(
			shared_ptr<parsing::define const> const& define_syntax)
	{
		return apply_visitor(define_generator(define_syntax->expression),
				*define_syntax->target);
	}

	struct statement_generator :
		public static_visitor<shared_ptr<typed_value const> >
	{
		shared_ptr<typed_value const> operator()(
				shared_ptr<parsing::define const> const& define) const
		{
			return generate_define(define);
		}

		shared_ptr<typed_value const> operator()(
				shared_ptr<parsing::expression const> const& expression) const
		{
			return generate_expression(expression);
		}
	};

	shared_ptr<typed_value const> generate_statement(
			shared_ptr<parsing::statement const> const& parse_statement)
	{
		return apply_visitor(statement_generator(), *parse_statement);
	}

	shared_ptr<typed_value const> generate_module(
			shared_ptr<parsing::module const> const& module_syntax)
	{
		using namespace primitives;

		shared_ptr<typed_cell const> args = nil();
		for (int i = int(module_syntax->defines.size()) - 1; i >= 0; --i)
			args = cons(generate_define(module_syntax->defines[i]), args);

		return value(cons(special_symbols::module, args));
	}
}}

namespace jest {namespace environment {
	using namespace std;
	using namespace boost;
	using namespace values;

	namespace detail
	{
		shared_ptr<binding const> default_env;
	}

	shared_ptr<typed_value const> lookup_default_binding(
			shared_ptr<typed_value const> const& symbol)
	{
		shared_ptr<values::binding const> match;
		for (shared_ptr<values::binding const> binding = detail::default_env;
				binding; binding = binding->tail)
		{
			if (binding->identifier == symbol)
			{
				match = binding;
				break;
			}
		}

		assert(match);
		return match->value;
	}

	void push_default_binding(shared_ptr<typed_value const> const& symbol,
			shared_ptr<typed_value const> const& value)
	{
		detail::default_env = make_shared<binding>(
				symbol, value, detail::default_env);
	}

	void push_operator(shared_ptr<typed_value const> const& identifier,
			scoping_policy scoping, evaluation_policy evaluation)
	{
		shared_ptr<values::operator_ const> new_operator =
			make_shared<values::operator_>(scoping, evaluation,
					shared_ptr<rule const>());
		push_default_binding(identifier, value(new_operator));
	}

	shared_ptr<binding const> get_default_environment()
	{return detail::default_env;}
}}

namespace jest {namespace native {

	using namespace boost;
	using namespace values;

	typedef function<shared_ptr<typed_value const>
		(shared_ptr<typed_cell const> const& args)> native_function;

	struct ellipsis
	{
		ellipsis(shared_ptr<typed_cell const> const& args): args(args) {}
		shared_ptr<typed_cell const> args;
	};

	template <typename T> void process_arg(
			shared_ptr<values::pattern const>& pattern,
			shared_ptr<typed_cell const>& expression,
			shared_ptr<typed_value const> const& symbol)
	{
		using namespace primitives;
		using namespace pattern_primitives;

		shared_ptr<void const> type_object = get_type_object<T>();

		// Create the pattern for matching this argument.
		shared_ptr<values::pattern const> arg_pattern =
			make_shared<values::pattern>(pattern_types::value,
					make_shared<values::pattern_value>(
						make_shared<values::pattern>(
							pattern_types::constant,
							make_shared<values::typed_value>(
								types::type, type_object)),
						make_shared<values::pattern>(
							pattern_types::variable, symbol)));
		pattern = pattern_cons(arg_pattern, pattern);

		// Create a reference to this argument in the expression.
		expression = cons(symbol, expression);
	}

	template <> void process_arg<ellipsis>(
			shared_ptr<values::pattern const>& pattern,
			shared_ptr<typed_cell const>& expression,
			shared_ptr<typed_value const> const& symbol)
	{
		using namespace primitives;
		using namespace pattern_primitives;

		// Create the pattern for matching the remainder of the arguments.
		shared_ptr<values::pattern const> arg_pattern =
			make_shared<values::pattern>(pattern_types::variable, symbol);

		// This should replace the tail of the list, rather than being in the
		// list (ie pattern list is improper).
		pattern = arg_pattern;

		// Create a reference to this argument in the expression.
		expression = cons(symbol, expression);
	}

	template <typename T> struct argument_type_of {typedef T type;};
	template <> struct argument_type_of<ellipsis> {typedef typed_cell type;};

	template <typename S> struct registrar {};

	template <typename T> struct referee_type_of {typedef T type;};
	template <typename X> struct referee_type_of<shared_ptr<X const> >
	{typedef X type;};

	template <typename X0> struct registrar<
		tuple<shared_ptr<typed_value const>, shared_ptr<binding const> >(*)(
				shared_ptr<binding const> const& environment,
				X0 const&)>
	{
		typedef typename referee_type_of<X0>::type referee_type;

		typedef tuple<shared_ptr<typed_value const>,
				shared_ptr<binding const> > signature(
				shared_ptr<binding const> const& environment,
				X0 const&);

		struct caller
		{
			caller(signature* fn): fn(fn) {}
			signature* fn;

			tuple<shared_ptr<typed_value const>,
					shared_ptr<binding const> > operator()(
					shared_ptr<binding const> const& environment,
					shared_ptr<typed_cell const> const& args) const
			{
				using namespace primitives;

				shared_ptr<typed_cell const> args_left = args;
				typedef typename argument_type_of<referee_type>::type a0;
				shared_ptr<a0 const> x0 = downcast<a0>(pop(args_left));
				assert(args_left == nil());
				return this->fn(environment, x0);
			}
		};

		void operator()(shared_ptr<typed_value const> const& symbol,
			signature* fn) const
		{
			using namespace primitives;
			using namespace pattern_primitives;
			using namespace environment;

			typedef tuple<shared_ptr<typed_value const>,
					shared_ptr<binding const> > erased_signature(
					shared_ptr<binding const> const& environment,
					shared_ptr<typed_cell const> const& args);

			shared_ptr<function<erased_signature> > caller_fn =
				make_shared<function<erased_signature> >(caller(fn));

			shared_ptr<values::operator_ const> old_operator =
				downcast<operator_>(lookup_default_binding(
							symbol));

			// Generate a rule to invoke the native form.
			shared_ptr<values::pattern const> pattern = pattern_nil();
			shared_ptr<values::typed_cell const> expression = nil();
			process_arg<referee_type>(pattern, expression, builtin_symbol("_0"));

			// Add 'native' and the function to call to the front of the 
			// expression.
			expression = cons(
					special_symbols::native,
					cons(
						make_shared<typed_value>(
							shared_ptr<void>(),
							caller_fn),
						expression));

			// Replace the old operator by rebinding a new one over the top.
			shared_ptr<values::operator_ const> new_operator =
				make_shared<values::operator_>(
						old_operator->scoping_policy,
						old_operator->evaluation_policy,
						make_shared<values::rule>(
							pattern,
							value(expression),
							shared_ptr<binding const>(),
							old_operator->rules));

			push_default_binding(symbol, value(new_operator));
		}
	};

	template <typename F> void register_(
			shared_ptr<typed_value const> const& symbol, F fn)
	{
		registrar<F>()(symbol, fn);
	}
}}

namespace jest {namespace evaluation {
	using namespace values;

	void fatal(char const* format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		fputs("\n", stderr);
		throw fatal_error();
	}

	shared_ptr<typed_value const> lookup_binding(
			shared_ptr<typed_value const> const& symbol,
			shared_ptr<binding const> const& env)
	{
		shared_ptr<values::binding const> match;
		for (shared_ptr<values::binding const> binding = env;
				binding; binding = binding->tail)
		{
			if (binding->identifier == symbol)
			{
				match = binding;
				break;
			}
		}

		assert(match);
		return match->value;
	}

	tuple<shared_ptr<typed_value const>, shared_ptr<binding const> > evaluate(
			shared_ptr<binding const> const& environment,
			shared_ptr<typed_value const> const& expression);

	tuple<shared_ptr<typed_cell const>, shared_ptr<binding const> >
		evaluate_args(
			shared_ptr<binding const> const& environment,
			shared_ptr<typed_cell const> const& args)
	{
		using namespace primitives;

		if (!args)
			return nil();

		shared_ptr<typed_value const> head_result;
		shared_ptr<binding const> head_bindings;
		tie(head_result, head_bindings) = evaluate(environment, args->head);

		shared_ptr<typed_cell const> tail_result;
		shared_ptr<binding const> tail_bindings;

		tie(tail_result, tail_bindings) = evaluate_args(head_bindings, args->tail);

		return make_tuple(cons(head_result, tail_result), tail_bindings);
	}

	tuple<shared_ptr<typed_value const>, shared_ptr<binding const> >
		apply_rules(
			scoping_policy scoping,
			shared_ptr<binding const> const& environment,
			shared_ptr<rule const> const& rules,
			shared_ptr<typed_cell const> const& args)
	{
		using namespace primitives;

		for (shared_ptr<rule const> rule = rules; rule; rule = rule->tail)
		{
			patterns::context c;
			shared_ptr<patterns::match_result const> match_result = match(
					&c, rule->pattern->type, rule->pattern->value,
					types::typed_cell, args);

			if (!match_result)
				continue;

			// Determine what bindings to pass, based on the scoping policy of 
			// the operator.
			shared_ptr<binding const> scope;
			switch (scoping)
			{
				case scoping_policy_dynamic: scope = environment; break;
				case scoping_policy_static: scope = rule->scope; break;
			}
			for (int i = 0, cnt = int(match_result->bindings.size());
					i < cnt; ++i)
			{
				shared_ptr<patterns::binding const> binding =
					match_result->bindings[i];

				scope = make_shared<values::binding>(
						binding->symbol,
						make_shared<typed_value>(
							binding->type, binding->value), scope);
			}

			return evaluate(scope, rule->expression);
		}

		assert(0);
		return make_tuple(shared_ptr<typed_value const>(),
				shared_ptr<binding const>());
	}

	tuple<shared_ptr<typed_value const>, shared_ptr<binding const> > evaluate(
			shared_ptr<binding const> const& environment,
			shared_ptr<typed_value const> const& expression)
	{
		using namespace primitives;

		if (expression->type == types::symbol)
			return make_tuple(
					lookup_binding(expression, environment), environment);

		if (expression->type != types::typed_cell)
		{
			fatal("Unable to evaluate expression.");
			return make_tuple(shared_ptr<typed_value const>(),
					shared_ptr<binding const>());
		}

		// Expressions of the form (native <fn> ...) should interpret <fn> as 
		// a function object to be passed the rest of the arguments.
		shared_ptr<typed_cell const> cell =
			static_pointer_cast<typed_cell const>(expression->value);
		if (cell->head == special_symbols::native)
		{
			// Extract the boost::function object from the list.
			typedef tuple<shared_ptr<typed_value const>,
					shared_ptr<binding const> > erased_signature(
					shared_ptr<binding const> const& environment,
					shared_ptr<typed_cell const> const& args);
			shared_ptr<function<erased_signature> const> caller_fn =
				static_pointer_cast<function<erased_signature> const>(
						cadr(cell)->value);

			// Call the native function via the function object.
			shared_ptr<typed_cell const> evaluated_args;
			shared_ptr<binding const> evaluated_bindings;
			tie(evaluated_args, evaluated_bindings) =
				evaluate_args(environment, cddr(cell));
			return (*caller_fn)(environment, evaluated_args);
		}
		else if (cell->head == special_symbols::quote)
		{
			assert(cddr(cell) == nil());
			return cadr(cell);
		}

		shared_ptr<typed_value const> operator_val;
		shared_ptr<binding const> operator_bindings;
	   	tie(operator_val, operator_bindings) =
				evaluate(environment, cell->head);

		if (operator_val->type == types::operator_)
		{
			shared_ptr<values::operator_ const> operator_ =
				static_pointer_cast<values::operator_ const>(
						operator_val->value);

			// Evaluate the args, unless the operator is a macro.
			shared_ptr<typed_cell const> evaluated_args;
			shared_ptr<binding const> args_bindings;
			switch (operator_->evaluation_policy)
			{
				case evaluation_policy_no_evaluate:
					evaluated_args = cell->tail;
					break;

				case evaluation_policy_evaluate:
					tie(evaluated_args, args_bindings) = evaluate_args(
						environment, cell->tail);
					break;
			}

			return apply_rules(operator_->scoping_policy,
					environment, operator_->rules, evaluated_args);
		}
		else
		{
			assert(0);
			return shared_ptr<typed_value const>();
		}
	}
}}

namespace jest {namespace builtin {namespace debugging {
	using namespace boost;
	using namespace values;
	using namespace primitives;

	tuple<shared_ptr<typed_value const>, shared_ptr<binding const> > print_symbol(
			shared_ptr<binding const> const& environment,
			shared_ptr<string const> const& symbol)
	{
		fputs(symbol->c_str(), stdout);
		return value(nil());
	}

	tuple<shared_ptr<typed_value const>, shared_ptr<binding const> > print_list(
			shared_ptr<binding const> const& environment,
			shared_ptr<typed_cell const> const& ls)
	{
		fputs("(", stdout);

		for (shared_ptr<typed_cell const> tail = ls; tail;
				tail = tail->tail)
		{
			evaluation::evaluate(environment,
					value(list(
							builtin_symbol("print"),
							value(list(special_symbols::quote,
									tail->head)))));

			if (tail->tail)
				fputs(" ", stdout);
		}

		fputs(")", stdout);

		return value(nil());
	}

	void register_functions()
	{
		environment::push_operator(builtin_symbol("print"),
				scoping_policy_dynamic, evaluation_policy_evaluate);
		native::register_(builtin_symbol("print"), print_symbol);
		native::register_(builtin_symbol("print"), print_list);
	}
}}}

namespace jest {namespace builtin {namespace modules {
	using namespace boost;
	using namespace values;
	using namespace primitives;

	tuple<shared_ptr<typed_value const>, shared_ptr<binding const> >
		evaluate_module(
			shared_ptr<binding const> const& environment,
			native::ellipsis const& rest)
	{
		return make_tuple(value(list(special_symbols::module)), environment);
	}

	void register_functions()
	{
		environment::push_operator(special_symbols::module,
				scoping_policy_dynamic, evaluation_policy_no_evaluate);
		native::register_(special_symbols::module, evaluate_module);
	}
}}}

int main(int /*argc*/, char* /*argv*/[])
{
	using namespace boost;
	using namespace jest;
	using namespace values;
	using namespace primitives;
	using namespace evaluation;
	using namespace environment;

	try
	{
		shared_ptr<parsing::module const> module_ast =
			parsing::parse_file("test/test.jest");

		shared_ptr<typed_value const> module_expression =
			generation::generate_module(module_ast);

		builtin::debugging::register_functions();
		builtin::modules::register_functions();

		//evaluate(get_default_environment(),
		//		value(list(
		//				builtin_symbol("print"),
		//				value(list(special_symbols::quote,
		//						module_expression)))));

		shared_ptr<typed_value const> module = evaluate(
				get_default_environment(),
				module_expression).get<0>();

		evaluate(get_default_environment(),
				value(list(
						builtin_symbol("print"),
						value(list(special_symbols::quote,
								module)))));

		printf("\n");
	}
	catch (fatal_error /*e*/)
	{
		fprintf(stderr, "Unrecoverable error; exiting.\n");
		return 1;
	}
}
