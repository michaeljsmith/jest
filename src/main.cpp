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

Value* f()
{
	return nil;
}

Value* literal(char const* s)
{
	return symbol(s);
}

Value* literal(Value* x)
{
	return x;
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
	if (car(entry) == symbol("define"))
	{
		if (sym == cadr(entry))
			return caddr(entry);
	}
	else if (car(entry) == symbol("module"))
	{
		Value* subenv = cadr(entry);
		Value* res = lookup_define_recurse(env, sym, subenv);
		if (res != _f)
			return res;
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
		Value* subenv = cadr(entry);
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

Value* evaluate_list_recurse(Value* env, Value* form)
{
	if (nil == form)
		return nil;
	return cons(evaluate(env, car(form)), evaluate_list_recurse(env, cdr(form)));
}

Value* evaluate_list(Value* env, Value* form)
{
	Value* scope = new Cell(symbol("module"), new Cell(nil, nil));
	Value* new_env = new Cell(scope, env);
	return evaluate_list_recurse(new_env, form);
}

Value* bind_matches(Value* env, Value* matches)
{
	if (matches == nil)
		return env;
	Value* sym = car(car(matches));
	Value* val = cadr(car(matches));
	return cons(list(symbol("define"), sym, val),
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

	set_car(new Cell(entry, cdr(car(env))), cdr(car(env)));
}

Value* evaluate(Value* env, Value* expr)
{
	if (stringp(expr))
		return expr;

	if (symbolp(expr))
		return lookup_define(env, expr);

	if (listp(expr))
	{
		if (expr != nil && car(expr) == symbol("quote"))
		{
			assert(cdr(expr) != nil);
			assert(cddr(expr) == nil);
			return cadr(expr);
		}

		if (expr != nil && car(expr) == symbol("define"))
		{
			Value* sym = cadr(expr);
			Value* val_expr = caddr(expr);
			Value* val = evaluate(env, val_expr);
			push_scope(list(symbol("define"), sym, val), env);
		}

		if (expr != nil && car(expr) == symbol("rule"))
		{
			Value* ptn = cadr(expr);
			Value* rule_expr = caddr(expr);
			assert(cdddr(expr) == nil);
			push_scope(list(symbol("rule"), list(ptn, rule_expr, env)), env);
		}

		return evaluate_pattern(env, expr);
	}

	assert(0);
	return _f;
}

int main(int /*argc*/, char* /*argv*/[])
{
	return 0;
}
