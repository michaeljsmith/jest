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
Value* int_ = _f;
Value* frame = _f;
Value* window = _f;
Value* label = _f;
Value* edit = _f;
Value* horizontal = _f;

Value* cons(Value* head, Value* tail)
{
	return new Cell(head, tail);
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

#include <map>
#include <string>
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
	return new String(text);
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

bool equal(Value* x0, Value* x1);

bool list_equal(Value* l0, Value* l1)
{
	if (0 == l0 && 0 == l1)
		return true;
	if (0 == l0 || 0 == l1)
		return false;
	return equal(car(l0), car(l1)) && list_equal(cdr(l0), cdr(l1));
}

bool equal(Value* x0, Value* x1)
{
	if (listp(x0) && listp(x1))
		return list_equal(x0, x1);
	if (listp(x0) || listp(x1))
		return false;
	return x0 == x1;
}

bool list_shallow_equal(Value* l0, Value* l1)
{
	if (0 == l0 && 0 == l1)
		return true;
	if (0 == l0 || 0 == l1)
		return false;
	return car(l0) == car(l1) && list_shallow_equal(cdr(l0), cdr(l1));
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

Value* map_car(Value* l)
{
	if (l == 0)
		return 0;
	return cons(
			car(car(l)),
			map_car(cdr(l)));
}

Value* map_cadr(Value* l)
{
	if (l == 0)
		return 0;
	return cons(
			cadr(car(l)),
			map_cadr(cdr(l)));
}

enum parsing_symbol
{
	p_eof,
	p_ident,
	p_lt,
	p_gt,
	p_colon,
	p_equal
};
struct context
{
	FILE* f;
	parsing_symbol symbol;
	char token[8096];
	char ch;
};

Value* parse_module(context* c);
Value* parse_define(context* c);
Value* parse_statement(context* c);
Value* parse_symbol_statement(context* c);
Value* parse_symbol_define(context* c);
Value* parse_form_statement(context* c);
Value* parse_form_statement_contents(context* c);
Value* parse_form_statement_tail(context* c);
Value* parse_non_thunk_tail(context* c);
Value* parse_define_tail(context* c);
Value* parse_form_expression_tail(context* c);
Value* parse_form_define(context* c);
Value* parse_parameter(context* c);
Value* parse_expression(context* c);
Value* parse_form_expression(context* c);

void read_symbol(context* c)
{
	while (isspace(c->ch))
		c->ch = fgetc(c->f);

	if (feof(c->f))
	{
		c->symbol = p_eof;
		c->token[0] = 0;
	}
	else if (c->ch == '<')
	{
		c->symbol = p_lt;
		strcpy(c->token, "<");
		c->ch = fgetc(c->f);
	}
	else if (c->ch == '>')
	{
		c->symbol = p_gt;
		strcpy(c->token, ">");
		c->ch = fgetc(c->f);
	}
	else if (c->ch == ':')
	{
		c->symbol = p_colon;
		strcpy(c->token, ":");
		c->ch = fgetc(c->f);
	}
	else if (c->ch == '=')
	{
		c->symbol = p_equal;
		strcpy(c->token, "=");
		c->ch = fgetc(c->f);
	}
	else if (isalnum(c->ch) || c->ch == '_')
	{
		c->symbol = p_ident;
		strcpy(c->token, "");
		while (isalnum(c->ch) || c->ch == '_')
		{
			char ch[2] = {c->ch, 0};
			strcat(c->token, ch);
			c->ch = fgetc(c->f);
		}
	}
	else
	{
		//fatal(c, "invalid character \'%c\'.", c->ch);
		assert(0);
		c->ch = fgetc(c->f);
	}
}

bool accept(context* c, parsing_symbol sym)
{
	if (c->symbol == sym)
	{
		read_symbol(c);
		return true;
	}
	return false;
}

void expect(context* c, parsing_symbol sym)
{
	if (c->symbol != sym)
		//fatal(c, "unexpected: %s", c->token.c_str());
		assert(0);
	read_symbol(c);
}

Value* parse_identifier(context* c)
{
	if (c->symbol == p_ident)
	{
		Value* id = symbol(c->token);
		read_symbol(c);
		return id;
	}
	return _f;
}

//module = {define}
Value* parse_module(context* c)
{
	Value* defines_reverse = 0;
	while (c->symbol != p_eof)
	{
		Value* define = parse_define(c);
		assert(define);
		assert(_f != define);
		defines_reverse = cons(define, defines_reverse);
	}
	return cons(symbol("module"), reverse(defines_reverse));
}

//define =
//	form_define
//	| symbol_define
Value* parse_define(context* c)
{
	Value* define = _f;
	define = (_f == define ? parse_form_define(c) : define);
	assert(define);
	define = (_f == define ? parse_symbol_define(c) : define);
	assert(define);
	return define;
}

//statement =
//	form_statement
//	| symbol_statement
Value* parse_statement(context* c)
{
	Value* statement = _f;
	statement = (_f == statement ? parse_form_statement(c) : statement);
	statement = (_f == statement ? parse_symbol_statement(c) : statement);
	return statement;
}

//symbol_statement =
//	identifier ["=" expression]
Value* parse_symbol_statement(context* c)
{
	Value* identifier = parse_identifier(c);
	if (_f == identifier)
		return _f;

	if (accept(c, p_equal))
	{
		Value* expression = parse_expression(c);
		return list(symbol("@member"), identifier, expression);
	}
	else
	{
		return identifier;
	}
}

//symbol_define =
//	identifier "=" expression
Value* parse_symbol_define(context* c)
{
	Value* identifier = parse_identifier(c);
	if (_f == identifier)
		return 0;

	expect(c, p_equal);

	Value* expression = parse_expression(c);

	return list(symbol("define"), identifier, expression);
}

//form_statement =
//	"<" form_statement_contents
Value* parse_form_statement(context* c)
{
	if (!accept(c, p_lt))
		return _f;
	return parse_form_statement_contents(c);
}

//form_statement_contents =
//	identifier form_statement_tail
//	| {statement} ">"
Value* parse_form_statement_contents(context* c)
{
	Value* identifier = parse_identifier(c);
	if (_f != identifier)
	{
		Value* tail = parse_form_statement_tail(c);

		assert(_f != tail);
		Value* field = car(tail);

		// Check whether we have parsed a new function.
		if (symbol("parameters") == car(field))
		{
			Value* parameters = cdr(field);
			Value* expression = cadr(tail);

			return list(symbol("@typefun"), identifier, parameters, expression);
		}
		// Check whether we have parsed a form.
		else if (symbol("statements") == car(field))
		{
			Value* statements = cdr(field);
			return cons(identifier, statements);
		}
		else
		{
			assert(0);
			return _f;
		}
	}
	else
	{
		Value* head = parse_expression(c);
		assert(head);

		Value* statements_reverse = 0;
		while (c->symbol != p_gt)
		{
			Value* statement = parse_statement(c);
			assert(statement);
			statements_reverse = cons(statement, statements_reverse);
		}

		expect(c, p_gt);

		return cons(head, reverse(statements_reverse));
	}
}

//form_statement_tail =
//	">" [define_tail]
//	| identifier non_thunk_tail
//	| {statement} ">"
Value* parse_form_statement_tail(context* c)
{
	if (accept(c, p_gt))
	{
		Value* expression = parse_define_tail(c);
		if (expression == _f)
			return list(list(symbol("statements")));
		else
			return list(list(symbol("parameters")), expression);
	}
	else
	{
		Value* identifier = parse_identifier(c);
		if (_f != identifier)
		{
			Value* tail = parse_non_thunk_tail(c);
			if (symbol("prototype") == car(tail))
			{
				Value* initial_type = cadr(tail);
				Value* expression = caddr(tail);
				Value* subsequent_parameters = cdddr(tail);

				return list(
						cons(
							symbol("parameters"),
							cons(list(initial_type, identifier),
								subsequent_parameters)),
						expression);
			}
			else if (symbol("form") == car(tail))
			{
				Value* statements = cdr(tail);
				return list(cons(symbol("statements"),
							cons(identifier, statements)));
			}
			else
			{
				assert(0);
				return _f;
			}
		}
		else
		{
			Value* statements_reverse = 0;
			while (c->symbol != p_gt)
			{
				Value* statement = parse_statement(c);
				assert(_f != statement);
				statements_reverse = cons(statement, statements_reverse);
			}
			expect(c, p_gt);

			return list(cons(symbol("statements"),
						reverse(statements_reverse)));
		}
	}
}

//non_thunk_tail =
//	":" expression {parameter} ">" define_tail
//	| {statement} ">"
Value* parse_non_thunk_tail(context* c)
{
	if (accept(c, p_colon))
	{
		Value* initial_type = parse_expression(c);

		Value* subsequent_reverse = 0;
		while (c->symbol != p_gt)
		{
			Value* parameter = parse_parameter(c);
			assert(_f != parameter);
			subsequent_reverse = cons(parameter, subsequent_reverse);
		}
		Value* subsequent_parameters = reverse(subsequent_reverse);

		expect(c, p_gt);

		Value* expression = parse_define_tail(c);
		return cons(symbol("prototype"),
				cons(initial_type,
					cons(expression,
						subsequent_parameters)));
	}
	else
	{
		Value* subsequent_reverse = 0;
		while (c->symbol != p_gt)
		{
			Value* statement = parse_statement(c);
			assert(_f != statement);
			subsequent_reverse = cons(statement, subsequent_reverse);
		}
		Value* subsequent_arguments = reverse(subsequent_reverse);

		expect(c, p_gt);

		return cons(symbol("form"), subsequent_arguments);
	}
}

//define_tail =
//	"=" expression
Value* parse_define_tail(context* c)
{
	if (!accept(c, p_equal))
		return _f;

	Value* expression = parse_expression(c);

	assert(_f != expression);

	return expression;
}

//form_expression_tail =
//	{statement} ">"
Value* parse_form_expression_tail(
		context* c)
{
	Value* statements_reverse = 0;
	while (c->symbol != p_gt)
	{
		Value* statement = parse_statement(c);
		assert(_f != statement);
		statements_reverse = cons(statement, statements_reverse);
	}

	expect(c, p_gt);

	return reverse(statements_reverse);
}

//form_define =
//	"<" expression {parameter} ">" = expression
Value* parse_form_define(context* c)
{
	if (!accept(c, p_lt))
		return _f;

	Value* name = parse_expression(c);
	assert(_f != name);

	Value* parameters_reverse = 0;
	while (c->symbol != p_gt)
	{
		Value* parameter = parse_parameter(c);
		assert(_f != parameter);
		parameters_reverse = cons(parameter, parameters_reverse);
	}
	Value* parameters = reverse(parameters_reverse);

	expect(c, p_gt);
	expect(c, p_equal);

	Value* expression = parse_expression(c);

	return list(symbol("@typefun"), name, parameters, expression);
}

//parameter =
//	identifier ":" identifier
Value* parse_parameter(context* c)
{
	Value* name = parse_identifier(c);
	assert(_f != name);

	expect(c, p_colon);

	Value* type = parse_expression(c);
	assert(_f != type);

	return list(type, name);
}

//expression =
//	identifier
//	| form_expression
Value* parse_expression(context* c)
{
	Value* identifier = parse_identifier(c);
	if (_f != identifier)
		return identifier;
	return parse_form_expression(c);
}

//form_expression =
//	"<" expression form_expression_tail
Value* parse_form_expression(context* c)
{
	if (!accept(c, p_lt))
		return _f;

	Value* head = parse_expression(c);
	assert(_f != head);

	Value* tail = parse_form_expression_tail(c);
	assert(_f != tail);

	return cons(head, tail);
}

Value* parse_file(char const* filename)
{
	context c;
	c.f = fopen(filename, "r");
	if (!c.f)
	{
		assert(0);
		//char buf[1024];
		//perror(buf);
		//fatal(&c, "error opening file \"%s\": %s", filename, buf);
	}

	c.ch = fgetc(c.f);
	read_symbol(&c);

	return parse_module(&c);
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

Value* lookup_binding_helper(Value* env, Value* expr)
{
	for (Value* current = env; current; current = cdr(current))
	{
		Value* entry = car(current);
		if (car(entry) == symbol("binding") && cadr(entry) == expr)
		{
			return entry;
		}
		else if (car(entry) == symbol("scope"))
		{
			Value* result = lookup_binding_helper(cadr(entry), expr);
			if (result != _f)
				return result;
		}
		//else if (car(entry) == symbol("module"))
		//{
		//	Value* result = lookup_binding_helper(cadr(entry), expr);
		//	if (result != _f)
		//		return result;
		//}
	}

	return _f;
}

Value* lookup_binding(Value* env, Value* expr)
{
	Value* binding = lookup_binding_helper(env, expr);
	return binding != _f ? caddr(binding) : _f;
}

Value* evaluate_compiletime(Value* env, Value* expr)
{
	if (symbolp(expr))
	{
		return lookup_binding(env, expr);
	}
	else if (consp(expr))
	{
		if (car(expr) == symbol("@get"))
		{
			Value* container = evaluate_compiletime(env, cadr(expr));
			if (car(container) == symbol("module"))
			{
				return lookup_binding(cadr(container), caddr(expr));
			}
			else
			{
				assert(0);
				return _f;
			}
		}
		else
		{
			assert(0);
			return _f;
		}
	}
	else
	{
		assert(0);
		return _f;
	}
}

Value* evaluate_type(Value* env, Value* expr)
{
	Value* type = evaluate_compiletime(env, expr);
	assert(type != _f);
	assert(car(type) == symbol("type"));
	return type;
}

Value* match_parameter_type(Value* pattern, Value* type)
{
	Value* pattern_type = car(pattern);
	assert(car(pattern_type) == symbol("type"));
	assert(car(type) == symbol("type"));
	return (pattern_type == type) ? 0 : _f;
}

Value* match_parameter_type_list_recurse(
		Value* pattern_list, Value* type_list)
{
	if (pattern_list == 0)
		return type_list == 0 ? 0 : _f;

	if (type_list == 0)
		return _f;

	Value* head_result = match_parameter_type(
			car(pattern_list), car(type_list));
	if (head_result == _f)
		return _f;
	return match_parameter_type_list_recurse(
			cdr(pattern_list), cdr(type_list));
}

Value* match_parameter_type_list(Value* pattern_list, Value* type_list)
{
	return match_parameter_type_list_recurse(pattern_list, type_list);
}

Value* get_type_debug_name(Value* type)
{
	Value* name = caddr(type);
	assert(stringp(name));
	return name;
}

Value* format_composite_name_recurse(Value* args)
{
	if (args == 0)
		return str("()");

	if (cdr(args) == 0)
		return concatenate(get_type_debug_name(car(args)), str(")"));

	return concatenate(concatenate(get_type_debug_name(car(args)), str(" ")),
			format_composite_name_recurse(cdr(args)));
}

Value* get_typefun_operator(Value* typefun)
{
	Value* operator_ = cadr(typefun);
	assert(car(operator_) == symbol("operator"));
	return operator_;
}

Value* get_operator_debug_name(Value* operator_)
{
	return cadr(operator_);
}

Value* format_composite_name(Value* typefun, Value* args)
{
	Value* operator_name = get_operator_debug_name(
			get_typefun_operator(typefun));

	return concatenate(concatenate(operator_name, str("(")),
			format_composite_name_recurse(args));
}

Value* evaluate_composite(
		Value* env, Value* name, Value* parameters, Value* expr, Value* scope);

Value* evaluate_typefun_form_recurse(Value* env, Value* scope, Value* form)
{
	for (Value* cur = scope; cur; cur = cdr(cur))
	{
		Value* entry = car(cur);

		if (car(entry) == symbol("module"))
		{
			Value* subres = evaluate_typefun_form_recurse(env, cadr(entry), form);
			if (subres != _f)
				return subres;
		}
		else if (car(entry) == symbol("scope"))
		{
			Value* subres = evaluate_typefun_form_recurse(env, cadr(entry), form);
			if (subres != _f)
				return subres;
		}
		else if (car(entry) == symbol("@typefun"))
		{
			if (car(form) != cadr(entry))
				continue;

			Value* match_res = match_parameter_type_list(
					caddr(entry), cdr(form));
			if (match_res != _f)
			{
				// TODO: Bind template parameters from match_res to
				// environment.
				Value* parameters = caddr(entry);
				Value* type = evaluate_composite(
						env, format_composite_name(entry, cdr(form)),
						parameters, cadddr(entry), caddddr(entry));
				return type;
			}
		}
	}

	return _f;
}

Value* evaluate_typefun_form(Value* env, Value* form)
{
	return evaluate_typefun_form_recurse(env, env, form);
}

Value* evaluate_operator(Value* env, Value* expr)
{
	Value* operator_ = evaluate_compiletime(env, expr);
	assert(operator_ != _f);
	assert(car(operator_) == symbol("operator"));
	return operator_;
}

Value* evaluate_parameter_types(Value* env,
		Value* parameters)
{
	if (!parameters)
		return 0;

	Value* parameter_expr = car(parameters);
	Value* parameter_type = car(parameter_expr);
	Value* parameter_name = cadr(parameter_expr);
	return cons(list(evaluate_type(env, parameter_type), parameter_name),
			evaluate_parameter_types(env, cdr(parameters)));
}

Value* compile_typefun_implicit_operator(
		Value* env, Value* typefun_expr)
{
	assert(car(typefun_expr) == symbol("@typefun"));

	Value* operator_expr = cadr(typefun_expr);
	if (symbolp(operator_expr))
	{
		// The name is a symbol. If this symbol is undefined, bind an operator
		// to it.
		if (lookup_binding(env, typefun_expr) != _f)
			return _f;

		return list(symbol("binding"), operator_expr,
				list(symbol("operator"), symbol_name(operator_expr)));
	}

	return _f;
}

Value* compile_typefun(Value* env, Value* typefun_expr)
{
	assert(car(typefun_expr) == symbol("@typefun"));

	Value* operator_expr = cadr(typefun_expr);
	Value* param_exprs = caddr(typefun_expr);
	Value* expr = cadddr(typefun_expr);

	Value* operator_ = evaluate_operator(env, operator_expr);
	Value* parameters = evaluate_parameter_types(env, param_exprs);
	return list(symbol("@typefun"), operator_, parameters, expr, env);
}

Value* evaluate_module(Value* expr)
{
	Value* env = default_env;

	assert(symbol("module") == car(expr));

	//Value* scope_sym = gensym();

	Value* module_entries = 0;
	for (Value* definitions = cdr(expr); definitions;
			definitions = cdr(definitions))
	{
		Value* definition = car(definitions);
		if (car(definition) == symbol("@typefun"))
		{
			// Create an implicit operator if required.
			Value* implicit_op_binding =
				compile_typefun_implicit_operator(env, definition);
			if (implicit_op_binding != _f)
			{
				module_entries = cons(implicit_op_binding, module_entries);
				env = cons(implicit_op_binding, env);
			}

			Value* typefun = compile_typefun(env, definition);
			module_entries = cons(typefun, module_entries);
			env = cons(typefun, env);
		}
		else
		{
			assert(0);
			return _f;
		}
	}

	//// Add the lexical scope var to the environment.
	//module_entries = cons(
	//		list(symbol("binding"), scope_sym, env), module_entries);

	return list(symbol("module"), module_entries);
}

Value* evaluate_member_subforms(Value* env, Value* composite, Value* subfms);

Value* evaluate_composite_member_form(
		Value* env, Value* composite, Value* form)
{
	Value* name = gensym();

	Value* child_info = evaluate_member_subforms(env, composite, cdr(form));
	Value* child_types = map_car(child_info);
	Value* child_syms = map_cadr(child_info);

	Value* operator_ = evaluate_compiletime(env, car(form));
	assert(operator_ != _f);

	Value* member_type = _f;
	if (car(operator_) == symbol("operator"))
	{
		member_type = evaluate_typefun_form(
				env, cons(operator_, child_types));
	}
	else if (car(operator_) == symbol("type"))
	{
		Value* type_arg_types = cadr(operator_);
		assert(list_shallow_equal(child_types, cdr(type_arg_types)));
		member_type = operator_;
	}
	else
	{
		assert(0);
	}

	Value* member = cons(
			symbol("member"),
			cons(
				name,
				cons(
					member_type,
					child_syms)));

	set_car(cdr(composite), cons(member, cadr(composite)));

	return member;
}

Value* evaluate_composite_member(Value* env, Value* composite, Value* expr)
{
	if (consp(expr))
	{
		return evaluate_composite_member_form(env, composite, expr);
	}
	else if (symbolp(expr))
	{
		Value* value = evaluate_compiletime(env, expr);
		assert(value != _f);
		if (car(value) == symbol("memberref"))
		{
			Value* member_composite = cadr(value);
			assert(car(member_composite) == symbol("composite"));
			assert(member_composite == composite);
			Value* member = caddr(value);
			assert(car(member) == symbol("member"));
			return member;
		}
		else
		{
			assert(0);
			return _f;
		}
	}
	else
	{
		assert(0);
		return _f;
	}
}

Value* bind_named_composite_member(
		Value* env, Value* scope_cell, Value* composite,
		Value* name, Value* expr)
{
	Value* member = evaluate_composite_member_form(env, composite, expr);
	set_car(scope_cell, cons(list(symbol("binding"), name,
				list(symbol("memberref"), composite, member)), car(scope_cell)));
	return member;
}

Value* evaluate_member_subforms_recurse(
		Value* env, Value* composite, Value* subfms, Value* scope)
{
	if (subfms == 0)
		return 0;

	Value* expr = car(subfms);

	Value* scope_cell = cdr(scope);

	// Check whether this is a nested function.
	if (consp(expr) && car(expr) == symbol("@typefun"))
	{
		// Create an implicit operator if required.
		Value* implicit_op_binding =
			compile_typefun_implicit_operator(env, expr);
		if (implicit_op_binding != _f)
			set_car(scope_cell, cons(implicit_op_binding, car(scope_cell)));

		Value* typefun = compile_typefun(env, expr);
		set_car(scope_cell, cons(typefun, car(scope_cell)));

		return evaluate_member_subforms_recurse(
				env, composite, cdr(subfms), scope);
	}

	// Check whether this is a named member.
	if (consp(expr) && car(expr) == symbol("@member"))
	{
		bind_named_composite_member(
				env, scope_cell, composite, cadr(expr), caddr(expr));
		return evaluate_member_subforms_recurse(
				env, composite, cdr(subfms), scope);
	}

	// Otherwise it is a positional argument.
	Value* child = evaluate_composite_member(env, composite, expr);
	assert(car(child) == symbol("member"));
	Value* name = cadr(child);
	Value* child_type = caddr(child);
	assert(car(child_type) == symbol("type"));
	Value* child_res = car(cadr(child_type));

	return cons(
			list(child_res, name),
			evaluate_member_subforms_recurse(
				env, composite, cdr(subfms), scope));
}

Value* evaluate_member_subforms(Value* env, Value* composite, Value* subfms)
{
	Value* scope = list(symbol("scope"), 0);
	env = cons(scope, env);
	return evaluate_member_subforms_recurse(env, composite, subfms, scope);
}

Value* bind_composite_parameters(
		Value*& env, Value* composite, Value* parameters, int idx)
{
	if (parameters == 0)
		return 0;

	Value* parameter = car(parameters);
	Value* type = car(parameter);
	Value* name = cadr(parameter);
	Value* member_name = gensym();

	char name_buf[1024];
	sprintf(name_buf, "arg%d", idx);
	Value* reference_type = list(symbol("type"), list(type), str(name_buf),
			list(symbol("argument"), symbol(name_buf)));
	Value* member = list(symbol("member"), member_name, reference_type);

	// Add the member ref to the env.
	env = cons(list(symbol("binding"), name,
				list(symbol("memberref"), composite, member)), env);

	return cons(member, bind_composite_parameters(
				env, composite, cdr(parameters), idx + 1));
}

Value* evaluate_composite(
		Value* env, Value* name, Value* parameters, Value* expr, Value* scope)
{
	Value* composite = (Value*)list(symbol("composite"), 0);
	Value* parameter_bindings = bind_composite_parameters(
			scope, composite, parameters, 0);
	set_car(cdr(composite), parameter_bindings);

	Value* root = evaluate_composite_member_form(scope, composite, expr);
	Value* root_type = caddr(root);
	Value* root_arg_types = cadr(root_type);

	return list(symbol("type"), cons(car(root_arg_types),
			map_cadr(parameters)), name, composite);
}

void register_primitive(Value*& env, char const* name)
{
	Value* type = list(symbol("type"), list(0), str(name));
	set_car(cadr(type), type);
	env = cons(list(symbol("binding"), symbol(name), type), env);
}

void register_type_list(Value*& env, char const* name, Value* args)
{
	Value* type = list(symbol("type"), args, str(name));
	env = cons(list(symbol("binding"), symbol(name), type), env);
}

Value* create_type_list(Value* env, char const* t, Value* tail)
{
	return cons(evaluate_type(env, symbol(t)), tail);
}

Value* create_type_list(
		Value* env, char const* t, char const* x0, Value* tail)
{
	return create_type_list(env, t,
			cons(evaluate_type(env, symbol(x0)), tail));
}

Value* create_type_list(Value* env, char const* t,
		char const* x0, char const* x1, Value* tail)
{
	return create_type_list(env, t, x0,
			cons(evaluate_type(env, symbol(x1)), tail));
}

void register_type(Value*& env, char const* name, char const* t,
		char const* x0)
{
	return register_type_list(env, name, create_type_list(env, t, x0, 0));
}

void register_type(Value*& env, char const* name, char const* t,
		char const* x0, char const* x1)
{
	return register_type_list(env, name, create_type_list(env, t, x0, x1, 0));
}

void initialize_default_environment()
{
	register_primitive(default_env, "int");
	register_primitive(default_env, "window");
	register_type(default_env, "label", "window", "int");
	register_type(default_env, "edit", "window", "int");
	register_type(default_env, "horizontal", "window", "window", "window");
	register_type(default_env, "frame", "window", "window");

	int_ = evaluate_type(default_env, symbol("int"));
	frame = evaluate_type(default_env, symbol("frame"));
	window = evaluate_type(default_env, symbol("window"));
	label = evaluate_type(default_env, symbol("label"));
	edit = evaluate_type(default_env, symbol("edit"));
	horizontal = evaluate_type(default_env, symbol("horizontal"));
}

Value* choose_struct_name(Value* type)
{
	static int next_id = 1;
	char buf[1024];
	sprintf(buf, "struct_%d", next_id++);
	return symbol(buf);
}

Value* choose_member_name(Value* id)
{
	static int next_id = 1;
	char buf[1024];
	sprintf(buf, "member_%d", next_id++);
	return symbol(buf);
}

Value* require_type(Value*& contents, Value* type);

Value* generate_members_recurse(Value*& contents, Value* members)
{
	if (0 == members)
		return 0;

	Value* content_tail = generate_members_recurse(contents, cdr(members));

	Value* member = car(members);
	Value* id = cadr(member);
	Value* type = caddr(member);
	Value* name = choose_member_name(id);

	Value* type_name = require_type(contents, type);

	assert(type_name != _f);
	if (type_name == 0)
		return content_tail;

	return cons(list(id, type_name, name), content_tail);
}

Value* generate_members(Value*& contents, Value* members)
{
	return generate_members_recurse(contents, members);
}

Value* generate_struct(Value*& contents, Value* type)
{
	assert(car(type) == symbol("type"));
	Value* composite = cadddr(type);
	assert(car(composite) == symbol("composite"));

	Value* name = choose_struct_name(type);

	Value* members = cadr(composite);
	Value* member_content = generate_members(contents, members);
	return list(name, list(symbol("struct"), name, member_content));
}

Value* lookup_contents_recurse(Value* contents, Value* name)
{
	if (contents == 0)
		return _f;

	Value* top = car(contents);
	if (car(top) == name)
		return cdr(top);

	return lookup_contents_recurse(cdr(contents), name);
}

Value* lookup_contents(Value* contents, Value* name)
{
	return lookup_contents_recurse(contents, name);
}

Value* require_type(Value*& contents, Value* type)
{
	// Don't try to generate built-in types.
	if (type == int_)
		return symbol("int");
	if (type == frame)
		return symbol("frame");
	if (type == window)
		return symbol("window");
	if (type == label)
		return symbol("label");
	if (type == edit)
		return symbol("edit");
	if (type == horizontal)
		return symbol("horizontal");

	// Don't try to generate argument reference types.
	Value* type_data = cadddr(type);
	if (consp(type_data) && car(type_data) == symbol("argument"))
		return 0;

	Value* entry = lookup_contents(contents, type);
	if (entry == _f)
	{
		entry = generate_struct(contents, type);
		Value* name = car(entry);
		Value* struct_ = cadr(entry);
		contents = cons(list(type, name, struct_), contents);
	}

	Value* name = car(entry);

	return name;
}

Value* find_typefun_code_recurse(Value* env, Value* scope, Value* form)
{
	for (Value* cur = scope; cur; cur = cdr(cur))
	{
		Value* entry = car(cur);

		if (car(entry) == symbol("module"))
		{
			Value* subres = find_typefun_code_recurse(env, cadr(entry), form);
			if (subres != _f)
				return subres;
		}
		else if (car(entry) == symbol("scope"))
		{
			Value* subres = find_typefun_code_recurse(env, cadr(entry), form);
			if (subres != _f)
				return subres;
		}
		else if (car(entry) == symbol("@typefun"))
		{
			if (car(form) != cadr(entry))
				continue;

			Value* match_res = match_parameter_type_list(
					caddr(entry), cdr(form));
			if (match_res != _f)
			{
				// TODO: Bind template parameters from match_res to
				// environment.
				//Value* parameters = caddr(entry);
//				Value* type = evaluate_composite(
//						env, format_composite_name(entry, cdr(form)),
//						parameters, cadddr(entry), caddddr(entry));
		//Value* env, Value* name, Value* parameters, Value* expr, Value* scope);
//				return type;
				assert(0);
			}
		}
	}

	return _f;
}

Value* generate_code(Value*& contents, Value* env, Value* context,
		Value* expression);

Value* bind_method_typefun_code_parameters_recurse(Value* bindings)
{
}

Value* lookup_method_typefun_binding_recurse(Value* name, Value* bindings)
{
	assert(0);
}

Value* determine_method_typefun_params_recurse(
		Value* type_bindings, Value* parameters)
{
	if (parameters == 0)
		return 0;

	return cons(
			parameter,
			determine_method_typefun_params_recurse(type_bindings,
				cdr(parameters)));
}

Value* generate_method(Value*& contents, Value* env, Value* operator_,
		Value* primitive, Value* types)
{
	assert(listp(expression));
	assert(symbolp(car(expression)));

	Value* form = cons(operator_, cons(primitive, expression));
	Value* typefun_binding_pair = find_typefun_code_recurse(env, env, form);
	assert(typefun_binding_pair != _f);

	Value* typefun = car(typefun_binding_pair);
	Value* template_bindings = cdr(typefun_binding_pair);

	Value* parameters = caddr(entry);
	Value* typefun_expr = cadddr(entry);
	Value* typefun_scope = caddddr(entry);

	// Add parameter references to context.

	// Evaluate code for expression.
	//Value* code = generate_code(contents, env, typefun_expr);
	assert(0);
}

Value* require_method(Value*& contents, Value* env, Value* operator_,
		Value* primitive, Value* types)
{
	assert(0);
}

Value* generate_code(Value*& contents, Value* env, Value* context,
		Value* expression)
{
	assert(0);
}

void write_struct_members_recurse(FILE* f, Value* members)
{
	if (members == 0)
		return;

	Value* member = car(members);
	Value* type_name = cadr(member);
	Value* name = caddr(member);

	fprintf(f, "    %s %s;\n", symbol_text(type_name), symbol_text(name));

	return write_struct_members_recurse(f, cdr(members));
}

void write_struct_members(FILE* f, Value* members)
{
	write_struct_members_recurse(f, members);
}

void write_struct(FILE* f, Value* struct_)
{
	assert(car(struct_) == symbol("struct"));
	Value* name = cadr(struct_);
	Value* members = caddr(struct_);

	fprintf(f, "typedef struct tag_%s\n", symbol_text(name));
	fprintf(f, "{\n");
	write_struct_members(f, members);
	fprintf(f, "} %s;\n\n", symbol_text(name));
}

void write_contents_recurse(FILE* f, Value* contents)
{
	if (contents == 0)
		return;

	write_contents_recurse(f, cdr(contents));

	Value* entry = car(contents);

	Value* entity = caddr(entry);

	if (car(entity) == symbol("struct"))
	{
		write_struct(f, entity);
	}
	else
	{
		assert(0);
	}
}

void write_contents(FILE* f, Value* contents)
{
	write_contents_recurse(f, contents);
}

int main(int /*argc*/, char* /*argv*/[])
{
	initialize_default_environment();

	Value* module_ast = parse_file("test/test2.jest");
	//debug_print(module_ast);
	//puts("");

	Value* module = evaluate_module(module_ast);

	Value* env = default_env;
	env = cons(module, env);
	env = cons(list(symbol("binding"), symbol("__main__"), module), env);

	Value* operator_ = evaluate_compiletime(env,
			list(symbol("@get"), symbol("__main__"), symbol("testui")));
	Value* composite = evaluate_typefun_form(env, list(operator_, int_));
	//debug_print(composite);
	//puts("");

	Value* contents = 0;
	require_type(contents, composite);

	//debug_print(contents);
	//puts("");

	write_contents(stdout, contents);

	return 0;
}

Current problem seems to be that types in typefuns are immediately evaluated - how will this work with template parameters? Probably needs to be a symbol reference.
