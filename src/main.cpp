#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <set>

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

namespace impl {struct BuiltinCaller;}
char const* builtin_type = "builtin";
struct Builtin : public Value
{
	impl::BuiltinCaller* caller;
	char const* name;

	template <typename S> Builtin(S* fn, char const* name);
	~Builtin();
};

Value* make_symbol(char const* s)
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

Value* make_cons(Value* head, Value* tail)
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

Value* _f = make_symbol("_f");
Value* _t = make_symbol("_t");

bool is_cons(Value* x)
{
	return x->type == cell_type;
}

bool is_list(Value* x)
{
	return 0 == x || is_cons(x);
}

bool is_string(Value* x)
{
	return x->type == string_type;
}

const char* text(Value* s)
{
	assert(is_string(s));
	return ((String*)s)->text;
}

bool is_symbol(Value* x)
{
	return x->type == symbol_type;
}

const char* get_symbol_text(Value* symbol)
{
	assert(is_symbol(symbol));
	return ((Symbol*)symbol)->sym;
}

bool is_builtin(Value* x)
{
	return x->type == builtin_type;
}

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

namespace impl
{
	Value* cons(Value* head, Value* tail)
	{
		return make_cons(head, tail);
	}

	Value* consp(Value* x)
	{
		return is_cons(x) ? _t : nil;
	}

	Value* listp(Value* x)
	{
		return is_list(x) ? _t : nil;
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
		assert(is_string(s));
		return make_symbol(((String*)s)->text);
	}

	Value* gensym()
	{
		static int next_id = 100;
		char str[1024];
		sprintf(str, "@gensym%d", next_id++);
		return make_symbol(str);
	}

	Value* symbolp(Value* x)
	{
		return is_symbol(x) ? _t : nil;
	}

	Value* builtinp(Value* x)
	{
		return is_builtin(x) ? _t : nil;
	}

	Value* symbol_text(Value* symbol)
	{
		return str(get_symbol_text(symbol));
	}

	Value* stringp(Value* x)
	{
		return is_string(x) ? _t : nil;
	}

	Value* symbol_name(Value* sym)
	{
		assert(symbolp(sym));
		return str(((Symbol*)sym)->sym);
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
		return list(make_symbol("quote"), expr);
	}

	Value* qq(Value* expr)
	{
		return list(make_symbol("quasiquote"), expr);
	}

	Value* uq(Value* expr)
	{
		return list(make_symbol("unquote"), expr);
	}

	Value* uqs(Value* expr)
	{
		return list(make_symbol("unquote-splicing"), expr);
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
}

namespace detail
{
	using namespace impl;

	struct Handle
	{
		Value* x;

		operator Value*() {return x;}

		Value* operator()() {return list(x);}
		template <typename X0> Value* operator()(X0 const& x0) {assert(0);return list(x, handle(x0));}
		template <typename X0, typename X1> Value* operator()(X0 const& x0, X1 const& x1) {return list(x, handle(x0), handle(x1));}
		template <typename X0, typename X1, typename X2> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2) {return list(x, handle(x0), handle(x1), handle(x2));}
		template <typename X0, typename X1, typename X2, typename X3> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3) {return list(x, handle(x0), handle(x1), handle(x2), handle(x3));}
		template <typename X0, typename X1, typename X2, typename X3, typename X4> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4) {return list(x, handle(x0), handle(x1), handle(x2), handle(x3), handle(x4));}
		template <typename X0, typename X1, typename X2, typename X3, typename X4, typename X5> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4, X5 const& x5) {return list(x, handle(x0), handle(x1), handle(x2), handle(x3), handle(x4), handle(x5));}

		template <typename Idx> Value* operator[](Idx idx) {return list(make_symbol("member"), x, handle(idx));}

		Handle(Value* x): x(x) {}
	};
}
using detail::Handle;

namespace impl
{
	Handle handle(Value* x) {return Handle(x);}
	Handle handle(char const* x) {return Handle(make_symbol(x));}

	struct BuiltinCaller
	{
		virtual ~BuiltinCaller() {}
		virtual Value* call(char const* fn_name, Value* args) = 0;
	};

	template <typename S> struct BuiltinCallerImpl {};

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

	template <> struct BuiltinCallerImpl<Value* ()> : public BuiltinCaller
	{
		Value* (*fn)();
		BuiltinCallerImpl(Value* (*fn)()): fn(fn) {}
		virtual Value* call(char const* fn_name, Value* args)
		{
			builtin_check_args(fn_name, 0, args);
			return fn();
		}
	};

	template <> struct BuiltinCallerImpl<Value* (Value*)> : public BuiltinCaller
	{
		Value* (*fn)(Value*);
		BuiltinCallerImpl(Value* (*fn)(Value*)): fn(fn) {}
		virtual Value* call(char const* fn_name, Value* args)
		{
			builtin_check_args(fn_name, 1, args);
			return fn(car(args));
		}
	};

	template <> struct BuiltinCallerImpl<Value* (Value*, Value*)> : public BuiltinCaller
	{
		Value* (*fn)(Value*, Value*);
		BuiltinCallerImpl(Value* (*fn)(Value*, Value*)): fn(fn) {}
		virtual Value* call(char const* fn_name, Value* args)
		{
			builtin_check_args(fn_name, 2, args);
			return fn(car(args), cadr(args));
		}
	};
}

using impl::BuiltinCallerImpl;

template <typename S> Builtin::Builtin(S* fn, char const* name)
	: Value(builtin_type), caller(new BuiltinCallerImpl<S>(fn)), name(name) {}
	Builtin::~Builtin() {delete caller;}

	namespace impl
{
	Value* call_builtin(Value* bi, Value* args)
	{
		assert(builtinp(bi));
		BuiltinCaller* caller = ((Builtin*)bi)->caller;
		char const* name = ((Builtin*)bi)->name;
		return caller->call(name, args);
	}
}

Value* builtins = nil;

namespace impl
{
	void push_builtin(char const* name, Value* val)
	{
		builtins = cons(list(make_symbol(name), val), builtins);
	}

	template <typename S> Value* define_builtin_fn(char const* name, S fn)
	{
		Value* builtin = new Builtin(fn, name);
		push_builtin(name, builtin);
		return q(builtin);
	}
}

using detail::define_builtin_fn;

#define BUILTIN(n) namespace prim {Handle n(define_builtin_fn(#n, impl::n));}
BUILTIN(cons);
BUILTIN(consp);
BUILTIN(listp);
BUILTIN(symbol);
BUILTIN(gensym);
BUILTIN(symbolp);
BUILTIN(builtinp);
BUILTIN(symbol_text);
BUILTIN(stringp);
BUILTIN(symbol_name);
BUILTIN(concatenate);
BUILTIN(car);
BUILTIN(cdr);
BUILTIN(cadr);
BUILTIN(cddr);
BUILTIN(caddr);
BUILTIN(cdddr);
BUILTIN(cadddr);
BUILTIN(cddddr);
BUILTIN(caddddr);
BUILTIN(append);
BUILTIN(reverse);
BUILTIN(q);
BUILTIN(qq);
BUILTIN(uq);
BUILTIN(uqs);
BUILTIN(assoc);
BUILTIN(evaluate);
#undef BUILTIN

namespace impl
{
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
		else if (is_symbol(x))
		{
			char const* str = ((Symbol*)x)->sym;
			FORMAT(str);
		}
		else if (is_string(x))
		{
			FORMAT(text(x));
		}
		else if (is_builtin(x))
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
}

namespace impl
{
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
			if (car(expr) == make_symbol("quote"))
			{
				builtin_check_args("quote", 1, cdr(expr));
				return cadr(expr);
			}
			Value* header = evaluate(env, car(expr));
			if (builtinp(header))
			{
				Value* args = map_evaluate(env, cdr(expr));
				return call_builtin(header, args);
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

//Handle prog = cons((str("a"), str("b")));
namespace prgm
{
	using namespace prim;
	Handle prog = cons(str("a"), str("b"));
}
using prgm::prog;

int main()
{
	using namespace impl;

	debug_print(prog);
	puts("");
	debug_print(evaluate(builtins, prog));
	puts("");
	return 0;
}
