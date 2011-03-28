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

Value* evaluate(Value* env, Value* expr);

Value* evaluate_scope_entry(Value** env, Value* expr)
{
	if (consp(expr))
	{
		Value* head_expr = car(expr);

		// Note: when creating new operators, make sure to create
		// operator-anchors to require attaching the scope on evaluation.
		if (head_expr == symbol("def"))
			return evaluate_def(env, cdr(expr));

		if (head_expr == symbol("prim"))
			return evaluate_prim(env, cdr(expr));

		if (head_expr == symbol("type"))
			return evaluate_type_def(env, cdr(expr));

		if (head_expr == symbol("fun"))
			return evaluate_fun_def(env, cdr(expr));
	}

	return evaluate(*env, expr);
}

Value* match_fun_pattern_recurse(Value* fun_arg, Value* arg, Value* matches)
{
	// Check whether we are matching a constant.
	if (symbolp(fun_arg))
		return (symbolp(arg) && arg == fun_arg ? matches : _f);

	// Check whether we are matching a variable.
	if (consp(fun_arg) && symbol("var") == car(fun_arg))
	{
		check(nilp(cdr(cdr(fun_arg))), "Pattern corrupt.");
		Value* name = cadr(fun_arg);
		Value* old = assoc(name, matches);
		if (old != _f && cadr(old) != arg)
			return _f;
		return cons(list(name, arg), matches);
	}
	// Check whether we are matching a list.
	else if (listp(fun_arg))
	{
		if (nilp(fun_args) && nilp(args))
			return matches;
		if (nilp(fun_args) || nilp(args))
			return _f;

		// Match the heads of the lists.
		matches = match_fun_pattern_recurse(car(fun_args), car(args), matches);
		if (matches == _f)
			return _f;

		// Now try matching the tails.
		return match_fun_pattern_recurse(cdr(fun_args), cdr(args), matches);
	}

	return error("Corrupt argument pattern."); 
}

Value* match_fun_pattern(Value* fun_op, Value* fun_args,
						 Value* op, Value* args)
{
	return match_fun_pattern_recurse(
			cons(fun_op, fun_args), cons(op, args), nil);
}

Value* bind_recurse(Value* env, Value* matches)
{
	if (nilp(matches))
		return env;
	return bind_recurse(
			cons(cons(symbol("def"), car(matches)), env), cdr(matches));
}

Value* bind_and_apply_fun(Value* env, Value* matches, Value* expr)
{
	evaluate(bind_recurse(env, matches), expr);
}

Value* find_and_apply_fun(Value* env, Value* operator_, Value* args)
{
	// TODO: in addition to the main env, we need to search the partial envs
	// of all the operators.
	
	// Check whether we have searched the entire stack and failed to find a
	// matching rule.
	if (env == nil)
		return _f;

	// TODO: Also check type funs!
	assert(0);

	// Check whether this is a fun that matches the expression.
	Value* entry = car(env);
	Value* fun = (consp(entry) && symbol("fun") == car(entry) ?
				  cdr(entry) : _f);
	check(_f == fun || (consp(fun) && consp(cdr(fun)) &&
						consp(consp(cdr(fun)))), "Function entry corrupt.");
	Value* matches = (_f != fun ? match_fun_pattern(
			car(fun), cadr(fun), car(operator_), args) : _f);
	if (_f != matches)
		return bind_and_apply_fun(cadddr(fun), matches, caddr(fun));

	// We didn't match, so recurse to the next entry on the stack.
	return find_and_apply_fun(cdr(env), operator_, args);
}

Value* find_matching_compound_and_apply_implicit_recurse(
		Value* env, Value* expr)
{
	// Check whether we have searched the entire stack and failed to find a
	// matching compound.
	if (env == nil)
		return _f;

	asdf;

	// We didn't match, so recurse to the next entry on the stack.
	return find_matching_compound_and_apply_implicit_recurse(cdr(env), expr);
}

Value* try_apply_implicit_fun(Value* env, Value* operator_, Value* args)
{
	// TODO: in addition to the main env, we need to search the partial envs
	// of all the operators.
	asdf;
	
	// Check whether we have searched the entire stack and failed to find a
	// matching type.
}

Value* evaluate_form(Value* env, Value* operator_, Value* arg_exprs)
{
	// Evaluate the args in a subscope.
	Value* args = evaluate_form_args(env, arg_exprs);

	Value* rslt = _f;

	// Look for a rule matching the expression (or a type expression).
	if (rslt == _f)
		rslt = find_and_apply_fun(env, operator_, args);

	// Try to apply an implicit rule based on a composite type.
	if (rslt == _f)
		rslt = try_apply_implicit_fun(env, operator_, args);

	if (rslt == _f)
		return error(list(string("Unable to evaluate form: "), cons(operator_, args)));

	return rslt;
}

Value* evaluate(Value* env, Value* expr)
{
	if (symbolp(expr))
		return evaluate_symbol(env, expr);

	if (stringp(expr))
		return expr;

	if (!listp(expr))
		return error(list(string("Unexpected value: "), expr));

	if (nilp(expr))
		return error(string("Unable to evaluate nil form"));

	Value* head_expr = car(expr);

	if (head_expr == symbol("def"))
		return evaluate_def(env, cdr(expr));

	if (head_expr == symbol("type"))
		return evaluate_type_def(env, cdr(expr));

	if (head_expr == symbol("fun"))
		return evaluate_fun_def(env, cdr(expr));

	Value* head = evaluate(env, head_expr);

	if (operatorp(head))
		return evaluate_type_form(env, head, cdr(expr));

	return error(list(string("Unable to evaluate form: "), expr));
}
