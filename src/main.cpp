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

bool consp(Value* x)
{
	return x->type == cell_type;
}

bool listp(Value* x)
{
	return 0 == x || consp(x);
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

Value* gensym()
{
	static int next_id = 100;
	char str[1024];
	sprintf(str, "@gensym%d", next_id++);
	return symbol(str);
}

bool symbolp(Value* x)
{
	return x->type == symbol_type;
}

const char* symbol_text(Value* symbol)
{
	assert(symbolp(symbol));
	return ((Symbol*)symbol)->sym;
}

bool stringp(Value* x)
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

Value* symbol_name(Value* sym)
{
	assert(symbolp(sym));
	return str(((Symbol*)sym)->sym);
}

const char* text(Value* s)
{
	assert(stringp(s));
	return ((String*)s)->text;
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

void set_car(Value* c, Value* x)
{
	assert(consp(x));
	((Cell*)c)->head = x;
}

Value* append(Value* l, Value* r)
{
	if (l == nil)
		return r;
	return cons(car(l), append(cdr(l), r));
}

Value* reverse_helper(Value* l, Value* x)
{
	if (0 == l)
		return x;
	assert(consp(l));
	return reverse_helper(cdr(l), cons(car(l), x));
}

Value* reverse(Value* x)
{
	return reverse_helper(x, 0);
}

Value* literal(char const* s)
{
	return symbol(s);
}

Value* literal(Value* x)
{
	return x;
}

Value* f()
{
	return nil;
}

template <typename X0> Value* f(X0 x0)
{
	return cons(literal(x0), f());
}

template <typename X0, typename X1> Value* f(X0 x0, X1 x1)
{
	return cons(literal(x0), f(x1));
}

Value* lookup_define_recurse(Value* env, Value* sym, Value* entries)
{
	if (env == nil)
		return _f;

	assert(listp(entries));

	Value* entry = car(entries);
	if (car(entry) == symbol("bind"))
	{
		if (sym == cadr(entry))
			return caddr(entry);
	}

	return lookup_define_recurse(env, sym, cdr(entries));
}

Value* lookup_define(Value* env, Value* symbol)
{
	return lookup_define_recurse(env, symbol, env);
}

Value* match_pattern(Value* pattern, Value* expr)
{
	if (symbolp(pattern))
		return pattern == expr ? nil : _f;

	if (nil == pattern)
		return expr == nil ? nil : _f;

	assert(consp(pattern));

	Value* head_matches = _f;
	if (symbol("unquote") == car(pattern))
	{
		assert(symbolp(cadr(pattern)));
		assert(nil == cddr(pattern));
		return list(list(cadr(pattern), expr));
	}
	else
	{
		if (!consp(expr))
			return _f;
		head_matches = match_pattern(car(pattern), car(expr));
	}

	if (head_matches == _f)
		return _f;

	Value* tail_matches = _f;
	if (listp(cdr(pattern)))
	{
		tail_matches = match_pattern(cdr(pattern), cdr(expr));
	}
	else
	{
		assert(consp(cdr(pattern)));
		assert(symbol("unquote") == car(cdr(pattern)));
		Value* sym = cadr(cdr(pattern));
		tail_matches = list(list(sym, cdr(expr)));
	}

	if (head_matches == _f || tail_matches == _f)
		return _f;

	return append(head_matches, tail_matches);
}

Value* find_pattern_match_recurse(Value* env, Value* expr, Value* entries)
{
	if (env == nil)
		return _f;

	assert(listp(entries));

	Value* entry = car(entries);
	if (car(entry) == symbol("rule"))
	{
		Value* rule = cadr(entry);
		Value* ptn = car(rule);
		Value* matches = match_pattern(ptn, expr);
		if (matches != _f)
			return list(matches, rule);
	}
	else if (car(entry) == symbol("module"))
	{
		Value* subenv = caddr(entry);
		Value* res = find_pattern_match_recurse(env, expr, subenv);
		if (res != _f)
			return res;
	}
	return find_pattern_match_recurse(env, expr, cdr(entries));
}

Value* find_pattern_match(Value* env, Value* expr)
{
	return find_pattern_match_recurse(env, expr, env);
}

Value* evaluate(Value* env, Value* expr);

Value* evaluate_list_recurse(Value* env, Value* list)
{
	if (nil == list)
		return nil;

	assert(consp(list));

	// Check whether we need to splice the child list into the parent list.
	if (consp(car(list)) && symbol("unquote-splicing") == car(car(list)))
		return append(evaluate(env, cadr(car(list))),
				evaluate_list_recurse(env, cdr(list)));
	else
		return cons(evaluate(env, car(list)), evaluate_list_recurse(env, cdr(list)));
}

Value* evaluate_list(Value* env, Value* form)
{
	return evaluate_list_recurse(env, form);
}

Value* bind_matches(Value* env, Value* matches)
{
	if (matches == nil)
		return env;
	Value* sym = car(car(matches));
	Value* val = cadr(car(matches));
	return cons(list(symbol("bind"), sym, val),
			bind_matches(env, cdr(matches)));
}

Value* evaluate_pattern(Value* env, Value* form)
{
	assert(form != nil);

	Value* evalfm = evaluate_list(env, form);
	Value* srch_res = find_pattern_match(env, evalfm);
	assert(srch_res != _f);
	if (srch_res == _f)
		return _f;

	Value* matches = car(srch_res);
	Value* rule = cadr(srch_res);
	Value* expr = cadr(rule);
	Value* rule_env = caddr(rule);

	Value* new_env = bind_matches(rule_env, matches);
	return evaluate(new_env, expr);
}

void push_scope(Value* entry, Value* env)
{
	assert(symbol("module") == car(car(env)));

	set_car(cddr(car(env)), new Cell(entry, caddr(car(env))));
}

Value* compile_pattern(Value* env, Value* ptn);

Value* compile_pattern_list(Value* env, Value* ptn)
{
	if (ptn == nil)
		return nil;

	if (!consp(ptn))
		return compile_pattern(env, ptn);

	return cons(compile_pattern(env, car(ptn)),
			compile_pattern_list(env, cdr(ptn)));
}

Value* compile_pattern(Value* env, Value* ptn)
{
	assert(ptn != nil);

	if (consp(ptn))
	{
		if (symbol("unquote") == car(ptn))
			return ptn;
		return compile_pattern_list(env, ptn);
	}

	if (symbolp(ptn))
		return evaluate(env, ptn);

	return ptn;
}

Value* evaluate(Value* env, Value* expr)
{
	if (stringp(expr))
		return expr;

	if (symbolp(expr))
		return lookup_define(env, expr);

	if (listp(expr))
	{
		// Evaluate quoted expressions.
		if (expr != nil && car(expr) == symbol("quote"))
		{
			assert(cdr(expr) != nil);
			assert(cddr(expr) == nil);
			return cadr(expr);
		}

		// Evaluate symbol bindings.
		if (expr != nil && car(expr) == symbol("bind"))
		{
			Value* sym = cadr(expr);
			Value* val_expr = caddr(expr);
			Value* val = evaluate(env, val_expr);
			push_scope(list(symbol("bind"), sym, val), env);
			return nil;
		}

		// Evaluate rule definitions.
		if (expr != nil && car(expr) == symbol("rule"))
		{
			Value* ptn = compile_pattern(env, cadr(expr));
			Value* rule_expr = caddr(expr);
			assert(cdddr(expr) == nil);
			push_scope(list(symbol("rule"), list(ptn, rule_expr, env)), env);
			return nil;
		}

		// Evaluate module member references.
		if (expr != nil && car(expr) == symbol(".mod"))
		{
			Value* mod_name = cadr(expr);
			Value* member = caddr(expr);

			Value* mod = lookup_define(env, mod_name);
			Value* contents = caddr(mod);
			return lookup_define(contents, member);
		}

		// Directly construct Cell instances, so that we don't wind up with a
		// shared instance (which would happen if we used cons()).
		Value* scope = new Cell(symbol("module"),
				new Cell(nil, new Cell(nil, nil)));
		Value* new_env = new Cell(scope, env);

		if (expr != nil && car(expr) == symbol("module"))
		{
			Value* mod_name = cadr(expr);
			set_car(cdr(scope), mod_name);
			evaluate_list(new_env, caddr(expr));

			// Grab the scope at the top of the stack - all the sub-defines
			// and rules have been added to it. This will now be the module
			// object.
			Value* mod = car(new_env);

			// Add the module to the scope, so that rules will automatically
			// be used.
			push_scope(mod, env);

			// Add the module as a define, so it can be accessed via name.
			push_scope(list(symbol("bind"), mod_name, mod), env);

			return mod;
		}

		return evaluate_pattern(new_env, expr);
	}

	assert(0);
	return _f;
}

int main(int /*argc*/, char* /*argv*/[])
{
	return 0;
}
