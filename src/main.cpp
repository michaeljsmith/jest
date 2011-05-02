#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <set>

namespace detail
{
	struct Value
	{
		Value(void const* type): type(type) {}
		void const* type;
	};

	Value* nil = 0;

	char const* cell_type = "cell";
	struct Cell : public Value
	{
		Cell(Value* head, Value* tail):
			Value(cell_type), head(head), tail(tail) {}

		Value* head;
		Value* tail;
	};

	char const* symbol_type = "symbol";
	struct Symbol : public Value
	{
		Symbol(char const* sym): Value(symbol_type)
		{
			this->sym = (char*)malloc(strlen(sym) + 1);
			strcpy(this->sym, sym);
		}

		char* sym;
	};

	char const* string_type = "string";
	struct String : public Value
	{
		String(char const* text): Value(string_type)
		{
			this->text = (char*)malloc(strlen(text) + 1);
			strcpy(this->text, text);
		}

		char* text;
	};

	char const* builtin_type = "builtin";
	typedef Value* (* BuiltinFn)(Value* env, Value* args);
	struct Builtin : public Value
	{
		BuiltinFn fn;
		char const* name;

		Builtin(BuiltinFn fn, char const* name)
			: Value(builtin_type), fn(fn), name(name) {}
	};
}

namespace jest
{
	using namespace detail;

	Value* nil = 0;

	Value* symbol(char const* s)
	{
		using namespace std;

		typedef map<string, Value*> SymMap;
		static SymMap symbol_map;
		string str(s);
		SymMap::iterator pos = symbol_map.find(str);
		if (pos == symbol_map.end())
		{
			Value* symbol = new Symbol(s);
			pos = symbol_map.insert(make_pair(str, symbol)).first;
		}

		return (*pos).second;
	}

	Value* cons(Value* head, Value* tail)
	{
		using namespace std;

		typedef map<std::pair<Value*, Value*>, Value*> CellMap;
		static CellMap cell_map;
		CellMap::iterator pos = cell_map.find(make_pair(head, tail));
		if (pos == cell_map.end())
		{
			Value* cell = new Cell(head, tail);
			pos = cell_map.insert(make_pair(make_pair(head, tail), cell)).first;
		}

		return (*pos).second;
	}

	namespace utils
	{
		Value* _f = symbol("_f");
		Value* _t = symbol("_t");

		Value* str(char const* text)
		{
			using namespace std;

			typedef map<string, Value*> SymMap;
			static SymMap symbol_map;
			string str(text);
			SymMap::iterator pos = symbol_map.find(str);
			if (pos == symbol_map.end())
			{
				Value* symbol = new String(text);
				pos = symbol_map.insert(make_pair(str, symbol)).first;
			}

			return (*pos).second;
		}
	}

	using namespace utils;

	bool consp(Value* x)
	{
		return x->type == cell_type;
	}

	bool listp(Value* x)
	{
		return 0 == x || consp(x);
	}

	bool stringp(Value* x)
	{
		return x->type == string_type;
	}

	const char* text(Value* s)
	{
		assert(stringp(s));
		return ((String*)s)->text;
	}

	bool symbolp(Value* x)
	{
		return x->type == symbol_type;
	}

	bool builtinp(Value* x)
	{
		return x->type == builtin_type;
	}

	Value* list()
	{
		return 0;
	}

	Value* list(Value* x0)
	{
		return cons(x0, list());
	}

	Value* list(Value* x0, Value* x1)
	{
		return cons(x0, list(x1));
	}

	Value* list(Value* x0, Value* x1, Value* x2)
	{
		return cons(x0, list(x1, x2));
	}

	Value* list(Value* x0, Value* x1, Value* x2, Value* x3)
	{
		return cons(x0, list(x1, x2, x3));
	}

	Value* list(Value* x0, Value* x1, Value* x2, Value* x3, Value* x4)
	{
		return cons(x0, list(x1, x2, x3, x4));
	}

	Value* list(Value* x0, Value* x1, Value* x2, Value* x3, Value* x4, Value* x5)
	{
		return cons(x0, list(x1, x2, x3, x4, x5));
	}

	Value* symbol(Value* s)
	{
		assert(stringp(s));
		return symbol(((String*)s)->text);
	}

	Value* gensym()
	{
		static int next_id = 100;
		char str[1024];
		sprintf(str, "@gensym%d", next_id++);
		return symbol(str);
	}

	Value* concatenate(Value* l, Value* r)
	{
		char const* tl = text(l);
		char const* tr = text(r);
		char* buf = new char [strlen(tl) + strlen(tr) + 1];
		strcpy(buf, tl);
		strcat(buf, tr);
		Value* res = str(buf);
		delete [] buf;
		return res;
	}

	Value* car(Value* x)
	{
		assert(consp(x));
		return ((Cell*)x)->head;
	}

	Value* cdr(Value* x)
	{
		assert(consp(x));
		return ((Cell*)x)->tail;
	}

	Value* cadr(Value* x)
	{
		assert(consp(x));
		return car(((Cell*)x)->tail);
	}

	Value* cddr(Value* x)
	{
		assert(consp(x));
		return cdr(((Cell*)x)->tail);
	}

	Value* caddr(Value* x)
	{
		assert(consp(x));
		return cadr(((Cell*)x)->tail);
	}

	Value* cdddr(Value* x)
	{
		assert(consp(x));
		return cddr(((Cell*)x)->tail);
	}

	Value* cadddr(Value* x)
	{
		assert(consp(x));
		return caddr(((Cell*)x)->tail);
	}

	Value* cddddr(Value* x)
	{
		assert(consp(x));
		return cdddr(((Cell*)x)->tail);
	}

	Value* caddddr(Value* x)
	{
		assert(consp(x));
		return cadddr(((Cell*)x)->tail);
	}

	Value* append(Value* l, Value* r)
	{
		if (l == nil)
			return r;
		return cons(car(l), append(cdr(l), r));
	}

	namespace detail
	{
		Value* reverse_helper(Value* l, Value* x)
		{
			if (0 == l)
				return x;
			assert(consp(l));
			return reverse_helper(cdr(l), cons(car(l), x));
		}
	}

	Value* reverse(Value* x)
	{
		return detail::reverse_helper(x, 0);
	}

	Value* q(Value* expr)
	{
		return list(symbol("quote"), expr);
	}

	Value* qq(Value* expr)
	{
		return list(symbol("quasiquote"), expr);
	}

	Value* uq(Value* expr)
	{
		return list(symbol("unquote"), expr);
	}

	Value* uqs(Value* expr)
	{
		return list(symbol("unquote-splicing"), expr);
	}

	Value* assoc(Value* a, Value* x)
	{
		if (a == nil)
			return nil;
		if (car(car(a)) == x)
			return car(a);
		return assoc(cdr(a), x);
	}

	Value* evaluate(Value* env, Value* expr);

	struct Handle
	{
		Value* x;

		operator Value*() {return x;}

		Value* operator()() {return list(x);}
		template <typename X0> Value* operator()(X0 const& x0) {return list(x, handle(x0));}
		template <typename X0, typename X1> Value* operator()(X0 const& x0, X1 const& x1) {return list(x, handle(x0), handle(x1));}
		template <typename X0, typename X1, typename X2> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2) {return list(x, handle(x0), handle(x1), handle(x2));}
		template <typename X0, typename X1, typename X2, typename X3> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3) {return list(x, handle(x0), handle(x1), handle(x2), handle(x3));}
		template <typename X0, typename X1, typename X2, typename X3, typename X4> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4) {return list(x, handle(x0), handle(x1), handle(x2), handle(x3), handle(x4));}
		template <typename X0, typename X1, typename X2, typename X3, typename X4, typename X5> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4, X5 const& x5) {return list(x, handle(x0), handle(x1), handle(x2), handle(x3), handle(x4), handle(x5));}

		template <typename Idx> Value* operator[](Idx idx) {return list(symbol("member"), x, handle(idx));}

		Handle(Value* x): x(x) {}
	};

	Handle handle(Value* x) {return Handle(x);}
	Handle handle(char const* x) {return Handle(symbol(x));}

	void builtin_check_args_recurse(char const* fn_name, int count, Value* args)
	{
		if (count == 0 && args == nil)
			return;

		if (args == nil)
		{
			printf("Too few args to function %s\n", fn_name);
			exit(1);
		}

		if (count == 0)
		{
			printf("Too many args for function %s\n", fn_name);
			exit(1);
		}

		builtin_check_args_recurse(fn_name, count - 1, cdr(args));
	}

	void builtin_check_args(char const* fn_name, int count, Value* args)
	{
		builtin_check_args_recurse(fn_name, count, args);
	}

	Value* call_builtin(Value* env, Value* bi, Value* args)
	{
		assert(builtinp(bi));
		BuiltinFn fn = ((Builtin*)bi)->fn;
		return fn(env, args);
	}

	Value* make_fntype(Value* args)
	{
		if (args == nil)
			printf("Missing return type for fntype.");
		return list(symbol("#fntype"), args);
	}

	namespace impl
	{
		Value* fntype(Value* env, Value* args)
		{
			return make_fntype(args);
		}
	}

	bool fntypep(Value* expr)
	{
		return consp(expr) && car(expr) == symbol("#fntype");
	}

	namespace impl
	{
		Value* fntypep(Value* env, Value* args)
		{
			builtin_check_args("fntypep", 1, args);
			return jest::fntypep(car(args)) ? _t : _f;
		}
	}

	Value* make_concept(Value* methods)
	{
		return list(symbol("#concept"), methods);
	}

	namespace impl
	{
		Value* concept(Value* env, Value* args)
		{
			return make_concept(args);
		}
	}

	Value* conceptp(Value* expr)
	{
		return consp(expr) && car(expr) == symbol("#concept");
	}

	namespace impl
	{
		Value* conceptp(Value* env, Value* args)
		{
			builtin_check_args("conceptp", 1, args);
			return jest::conceptp(car(args)) ? _t : _f;
		}
	}

	Value* builtins = nil;

	Value* push_builtin(char const* name, Value* val)
	{
		builtins = cons(list(symbol(name), val), builtins);
		return val;
	}

	Value* define_builtin_fn(char const* name, BuiltinFn fn)
	{
		Value* builtin = new Builtin(fn, name);
		push_builtin(name, builtin);
		return q(builtin);
	}

	namespace dsl
	{
		using namespace utils;
	}

#define BUILTIN(n) namespace dsl {Handle n(define_builtin_fn(#n, impl::n));}
	BUILTIN(fntype);
	BUILTIN(fntypep);
#undef BUILTIN

	Value* map_evaluate(Value* env, Value* list)
	{
		if (list == nil)
			return nil;
		return cons(evaluate(env, car(list)), map_evaluate(env, cdr(list)));
	}

#define FORMAT(x) do \
	{char const* s = (x); strcpy(buf, s); buf += strlen(s);} while(0)
	void debug_format_recurse(
			char*& buf, Value* x, std::set<Value*>& printed_cells)
	{
		if (0 == x)
		{
			FORMAT("()");
		}
		else if (_f == x)
		{
			FORMAT("_f");
		}
		else if (consp(x))
		{
			if (printed_cells.find(x) != printed_cells.end())
			{
				FORMAT("(...)");
			}
			else
			{
				printed_cells.insert(x);

				FORMAT("(");
				for (Value* l = x; l; l = cdr(l))
				{
					if (consp(l))
					{
						debug_format_recurse(buf, car(l), printed_cells);
						if (cdr(l))
							FORMAT(" ");
					}
					else
					{
						FORMAT(" . ");
						debug_format_recurse(buf, l, printed_cells);
						break;
					}
				}
				FORMAT(")");
			}
		}
		else if (symbolp(x))
		{
			char const* str = ((Symbol*)x)->sym;
			FORMAT(str);
		}
		else if (stringp(x))
		{
			FORMAT(text(x));
		}
		else if (builtinp(x))
		{
			char const* name = ((Builtin*)x)->name;
			FORMAT(name);
		}
	}
#undef FORMAT

	void debug_print(Value* x)
	{
		char buffer[1000000];
		char* buf = buffer;

		std::set<Value*> printed_cells;

		debug_format_recurse(buf, x, printed_cells);
		fputs(buffer, stdout);
	}

	char* debug_format(Value* x)
	{
		static char buffer[1000000];
		char* buf = buffer;

		std::set<Value*> printed_cells;

		debug_format_recurse(buf, x, printed_cells);
		return buffer;
	}

	Value* evaluate(Value* env, Value* expr)
	{
		if (stringp(expr))
		{
			return expr;
		}
		else if (symbolp(expr))
		{
			Value* entry = assoc(env, expr);
			if (entry == nil)
			{
				printf("Undefined symbol ");
				debug_print(expr);
				printf("\n");
				exit(1);
			}
			return cadr(entry);
		}
		else if (listp(expr))
		{
			if (car(expr) == symbol("quote"))
			{
				builtin_check_args("quote", 1, cdr(expr));
				return cadr(expr);
			}

			Value* header = evaluate(env, car(expr));
			if (builtinp(header))
			{
				return call_builtin(env, header, cdr(expr));
			}
			else
			{
				assert(0);
			}
			return nil;
		}
		else
		{
			return nil;
		}
	}
}

namespace prgm
{
	using namespace jest::dsl;
	//jest::Handle prog = consp(str("a"), str("b"));
	jest::Handle prog = fntype(str("a"), str("b"));
}
using prgm::prog;

int main()
{
	using namespace jest;

	debug_print(prog);
	puts("");
	debug_print(evaluate(builtins, prog));
	puts("");
	return 0;
}
