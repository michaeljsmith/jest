#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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

Value* _f = new Value(0);

Value* default_env = 0;

#include <map>
#include <string>
Value* cons_b(Value* head, Value* tail)
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

bool consp_b(Value* x)
{
	return x->type == cell_type;
}

bool listp_b(Value* x)
{
	return 0 == x || consp_b(x);
}

Value* list_b()
{
	return 0;
}

Value* list_b(Value* x0)
{
	return cons_b(x0, list_b());
}

Value* list_b(Value* x0, Value* x1)
{
	return cons_b(x0, list_b(x1));
}

Value* list_b(Value* x0, Value* x1, Value* x2)
{
	return cons_b(x0, list_b(x1, x2));
}

Value* list_b(Value* x0, Value* x1, Value* x2, Value* x3)
{
	return cons_b(x0, list_b(x1, x2, x3));
}

Value* list_b(Value* x0, Value* x1, Value* x2, Value* x3, Value* x4)
{
	return cons_b(x0, list_b(x1, x2, x3, x4));
}

Value* list_b(Value* x0, Value* x1, Value* x2, Value* x3, Value* x4, Value* x5)
{
	return cons_b(x0, list_b(x1, x2, x3, x4, x5));
}

Value* symbol_b(char const* s)
{
	using namespace std;

	typedef map<string, Value*> SymMap;
	static SymMap symbol_map;
	string str(s);
	SymMap::iterator pos = symbol_map.find(str);
	if (pos == symbol_map.end())
	{
		Value* symbol_b = new Symbol(s);
		pos = symbol_map.insert(make_pair(str, symbol_b)).first;
	}

	return (*pos).second;
}

struct Handle
{
    Value* x;

    operator Value*() {return x;}

    Value* operator()() {return list_b(x);}
    template <typename X0> Value* operator()(X0 const& x0) {assert(0);return list_b(x, handle(x0));}
    template <typename X0, typename X1> Value* operator()(X0 const& x0, X1 const& x1) {return list_b(x, handle(x0), handle(x1));}
    template <typename X0, typename X1, typename X2> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2) {return list_b(x, handle(x0), handle(x1), handle(x2));}
    template <typename X0, typename X1, typename X2, typename X3> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3) {return list_b(x, handle(x0), handle(x1), handle(x2), handle(x3));}
    template <typename X0, typename X1, typename X2, typename X3, typename X4> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4) {return list_b(x, handle(x0), handle(x1), handle(x2), handle(x3), handle(x4));}
    template <typename X0, typename X1, typename X2, typename X3, typename X4, typename X5> Value* operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4, X5 const& x5) {return list_b(x, handle(x0), handle(x1), handle(x2), handle(x3), handle(x4), handle(x5));}

    template <typename Idx> Value* operator[](Idx idx) {return list_b(symbol_b("member"), x, handle(idx));}

    Handle(Value* x): x(x) {}
};

Handle handle(Value* x) {return Handle(x);}
Handle handle(char const* x) {return Handle(symbol_b(x));}

Value* gensym()
{
	static int next_id = 100;
	char str[1024];
	sprintf(str, "@gensym%d", next_id++);
	return symbol_b(str);
}

bool symbolp_b(Value* x)
{
	return x->type == symbol_type;
}

const char* symbol_text(Value* symbol)
{
	assert(symbolp_b(symbol));
	return ((Symbol*)symbol)->sym;
}

bool stringp_b(Value* x)
{
	return x->type == string_type;
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

Value* symbol_name_b(Value* sym)
{
	assert(symbolp_b(sym));
	return str(((Symbol*)sym)->sym);
}

const char* text(Value* s)
{
	assert(stringp_b(s));
	return ((String*)s)->text;
}

Value* concatenate_b(Value* l, Value* r)
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

Value* car_b(Value* x)
{
	assert(consp_b(x));
	return ((Cell*)x)->head;
}

Value* cdr_b(Value* x)
{
	assert(consp_b(x));
	return ((Cell*)x)->tail;
}

Value* cadr_b(Value* x)
{
	assert(consp_b(x));
	return car_b(((Cell*)x)->tail);
}

Value* cddr_b(Value* x)
{
	assert(consp_b(x));
	return cdr_b(((Cell*)x)->tail);
}

Value* caddr_b(Value* x)
{
	assert(consp_b(x));
	return cadr_b(((Cell*)x)->tail);
}

Value* cdddr_b(Value* x)
{
	assert(consp_b(x));
	return cddr_b(((Cell*)x)->tail);
}

Value* cadddr_b(Value* x)
{
	assert(consp_b(x));
	return caddr_b(((Cell*)x)->tail);
}

Value* cddddr_b(Value* x)
{
	assert(consp_b(x));
	return cdddr_b(((Cell*)x)->tail);
}

Value* caddddr_b(Value* x)
{
	assert(consp_b(x));
	return cadddr_b(((Cell*)x)->tail);
}

Value* append_b(Value* l, Value* r)
{
	if (l == nil)
		return r;
	return cons_b(car_b(l), append_b(cdr_b(l), r));
}

Value* reverse_helper(Value* l, Value* x)
{
	if (0 == l)
		return x;
	assert(consp_b(l));
	return reverse_helper(cdr_b(l), cons_b(car_b(l), x));
}

Value* reverse_b(Value* x)
{
	return reverse_helper(x, 0);
}

Value* q_b(Value* expr)
{
	return list_b(symbol_b("quote"), expr);
}

Value* qq_b(Value* expr)
{
	return list_b(symbol_b("quasiquote"), expr);
}

Value* uq_b(Value* expr)
{
	return list_b(symbol_b("unquote"), expr);
}

Value* uqs_b(Value* expr)
{
	return list_b(symbol_b("unquote-splicing"), expr);
}

struct BuiltinCaller
{
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

	builtin_check_args_recurse(fn_name, count - 1, cdr_b(args));
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
        return fn(car_b(args));
    }
};

template <> struct BuiltinCallerImpl<Value* (Value*, Value*)> : public BuiltinCaller
{
    Value* (*fn)(Value*, Value*);
    BuiltinCallerImpl(Value* (*fn)(Value*, Value*)): fn(fn) {}
    virtual Value* call(char const* fn_name, Value* args)
    {
		builtin_check_args(fn_name, 2, args);
        return fn(car_b(args), cadr_b(args));
    }
};

char const* builtin_type = "builtin";
struct Builtin : public Value
{
    BuiltinCaller* caller;
	char const* name;

    template <typename S> Builtin(S* fn, char const* name)
		: Value(builtin_type), caller(new BuiltinCallerImpl<S>(fn)), name(name) {}
    ~Builtin() {delete caller;}
};

bool builtinp_b(Value* x)
{
	return x->type == builtin_type;
}

Value* call_builtin(Value* bi, Value* args)
{
    assert(builtinp_b(bi));
    BuiltinCaller* caller = ((Builtin*)bi)->caller;
	char const* name = ((Builtin*)bi)->name;
    return caller->call(name, args);
}

Value* builtins = nil;

void push_builtin(char const* name, Value* val)
{
    builtins = cons_b(list_b(symbol_b(name), val), builtins);
}

template <typename S> Value* define_builtin_fn(char const* name, S fn)
{
	Value* builtin = new Builtin(fn, name);
	push_builtin(name, builtin);
	return q_b(builtin);
}

#define BUILTIN(n) Handle n(define_builtin_fn(#n, n##_b))
BUILTIN(cons);

Value* assoc_b(Value* a, Value* x)
{
    if (a == nil)
        return nil;
    if (car_b(car_b(a)) == x)
        return car_b(a);
    return assoc_b(cdr_b(a), x);
}

Value* evaluate_b(Value* env, Value* expr);

Value* map_evaluate(Value* env, Value* list)
{
    if (list == nil)
        return nil;
    return cons_b(evaluate_b(env, car_b(list)), map_evaluate(env, cdr_b(list)));
}

#include <set>
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
	else if (consp_b(x))
	{
		if (printed_cells.find(x) != printed_cells.end())
		{
			FORMAT("(...)");
		}
		else
		{
			printed_cells.insert(x);

			FORMAT("(");
			for (Value* l = x; l; l = cdr_b(l))
			{
				if (consp_b(l))
				{
					debug_format_recurse(buf, car_b(l), printed_cells);
					if (cdr_b(l))
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
	else if (symbolp_b(x))
	{
		char const* str = ((Symbol*)x)->sym;
		FORMAT(str);
	}
	else if (stringp_b(x))
	{
		FORMAT(text(x));
	}
	else if (builtinp_b(x))
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

Value* evaluate_b(Value* env, Value* expr)
{
    if (stringp_b(expr))
    {
        return expr;
    }
    else if (symbolp_b(expr))
    {
        Value* entry = assoc_b(env, expr);
        if (entry == nil)
        {
            printf("Undefined symbol ");
            debug_print(expr);
            printf("\n");
            exit(1);
        }
        return cadr_b(entry);
    }
    else if (listp_b(expr))
    {
		if (car_b(expr) == symbol_b("quote"))
		{
			builtin_check_args("quote", 1, cdr_b(expr));
			return cadr_b(expr);
		}
		Value* header = evaluate_b(env, car_b(expr));
        if (builtinp_b(header))
		{
			Value* args = map_evaluate(env, cdr_b(expr));
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

//Handle prog = cons((str("a"), str("b")));
Handle prog = cons(str("a"), str("b"));

int main()
{
		debug_print(prog);
		puts("");
    debug_print(evaluate_b(builtins, prog));
    puts("");
    return 0;
}
