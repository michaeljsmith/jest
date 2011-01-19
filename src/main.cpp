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

Value* cons(Value* head, Value* tail)
{
	return new Cell(head, tail);
}

bool consp(Value* x)
{
	return x->type == cell_type;
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

void set_car(Value* c, Value* x)
{
	assert(consp(x));
	((Cell*)c)->tail = x;
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
	define = (_f == define ? parse_symbol_define(c) : define);
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
		return list(symbol("member"), identifier, expression);
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

			return list(symbol("typefun"), identifier, parameters, expression);
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
	if (c->symbol == p_gt)
	{
		Value* expression = parse_define_tail(c);
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

	return list(symbol("typefun"), name, parameters, expression);
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
void debug_print_recurse(Value* x, std::set<Value*>& printed_cells)
{
	if (0 == x)
	{
		printf("()");
	}
	else if (consp(x))
	{
		if (printed_cells.find(x) != printed_cells.end())
		{
			printf("(...)");
		}
		else
		{
			printed_cells.insert(x);

			printf("(");
			for (Value* l = x; l; l = cdr(l))
			{
				if (consp(l))
				{
					debug_print_recurse(car(l), printed_cells);
					if (cdr(l))
						printf(" ");
				}
				else
				{
					printf(" . ");
					debug_print_recurse(l, printed_cells);
					break;
				}
			}
			printf(")");
		}
	}
	else if (symbolp(x))
	{
		char const* str = ((Symbol*)x)->sym;
		fputs(str, stdout);
	}
	else if (stringp(x))
	{
		fputs(text(x), stdout);
	}
}

void debug_print(Value* x)
{
	std::set<Value*> printed_cells;

	debug_print_recurse(x, printed_cells);
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
		if (car(expr) == symbol("get"))
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

Value* evaluate_composite_form(Value* env, Value* name, Value* expr);

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
		else if (car(entry) == symbol("typefun"))
		{
			if (car(form) != cadr(entry))
				continue;

			Value* match_res = match_parameter_type_list(
					caddr(entry), cdr(form));
			if (match_res != _f)
			{
				Value* type = evaluate_composite_form(
						env, format_composite_name(entry, cdr(form)),
						cadddr(entry));
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
	assert(car(typefun_expr) == symbol("typefun"));

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

Value* compile_typefun(Value* env, Value* scope_sym,
		Value* typefun_expr)
{
	assert(car(typefun_expr) == symbol("typefun"));

	Value* operator_expr = cadr(typefun_expr);
	Value* param_exprs = caddr(typefun_expr);
	Value* expr = cadddr(typefun_expr);

	Value* operator_ = evaluate_operator(env, operator_expr);
	Value* parameters = evaluate_parameter_types(env, param_exprs);
	return list(symbol("typefun"), operator_, parameters, expr);
}

Value* evaluate_module(Value* expr)
{
	Value* env = default_env;

	assert(symbol("module") == car(expr));

	Value* scope_sym = gensym();

	Value* module_entries = 0;
	for (Value* definitions = cdr(expr); definitions;
			definitions = cdr(definitions))
	{
		Value* definition = car(definitions);
		if (car(definition) == symbol("typefun"))
		{
			// Create an implicit operator if required.
			Value* implicit_op_binding =
				compile_typefun_implicit_operator(env, definition);
			if (implicit_op_binding != _f)
			{
				module_entries = cons(implicit_op_binding, module_entries);
				env = cons(implicit_op_binding, env);
			}

			Value* typefun = compile_typefun(
					env, scope_sym, definition);
			module_entries = cons(typefun, module_entries);
			env = cons(typefun, env);
		}
		else
		{
			assert(0);
			return _f;
		}
	}

	// Add the lexical scope var to the environment.
	module_entries = cons(
			list(symbol("binding"), scope_sym, env), module_entries);

	return list(symbol("module"), module_entries);
}

Value* evaluate_composite_member(
		Value* env, Value* name,
		Value* composite, Value* form);

Value* evaluate_member_subforms(
		Value* env, Value* type, Value* subfms)
{
	if (subfms == 0)
		return 0;

	Value* name = gensym();
	Value* child = evaluate_composite_member(
			env, name, type, car(subfms));
	Value* child_type = caddr(child);
	assert(car(child_type) == symbol("type"));
	Value* child_res = car(cadr(child_type));

	return cons(
			cons(child_res, name),
			evaluate_member_subforms(env, type, cdr(subfms)));
}

Value* evaluate_composite_member_form(
		Value* env, Value* name,
		Value* composite, Value* form)
{
	Value* child_info = evaluate_member_subforms(
			env, composite, cdr(form));
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
		assert(child_syms == 0);
		member_type = operator_;
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

Value* evaluate_composite_member(
		Value* env, Value* name,
		Value* composite, Value* expr)
{
	if (consp(expr))
	{
		return evaluate_composite_member_form(env, name, composite, expr);
	}
	else if (symbolp(expr))
	{
		Value* type = evaluate_type(env, expr);
		Value* member = cons(
				symbol("member"),
				cons(name, type));
		return member;
	}
	else
	{
		assert(0);
		return _f;
	}
}

Value* evaluate_composite_form(Value* env, Value* name, Value* expr)
{
	Value* composite = (Value*)list(symbol("composite"), 0);

	Value* root = evaluate_composite_member_form(
			env, gensym(), composite, expr);
	Value* root_type = caddr(root);
	Value* root_arg_types = cadr(root_type);

	return list(symbol("type"), root_arg_types, name, composite);
}

void initialize_default_environment()
{
	// Declare int.
	Value* int_ = list(symbol("type"), list(0), str("int"));
	set_car(cadr(int_), int_);
	default_env = cons(
			list(symbol("binding"), symbol("int"), int_), default_env);
}

int main(int /*argc*/, char* /*argv*/[])
{
	initialize_default_environment();

	Value* module_ast = parse_file("test/test2.jest");
	debug_print(module_ast);
	puts("");

	Value* module = evaluate_module(module_ast);

	Value* env = default_env;
	env = cons(module, env);
	env = cons(list(symbol("binding"), symbol("__main__"), module), env);

	Value* operator_ = evaluate_compiletime(env,
			list(symbol("get"), symbol("__main__"), symbol("testui")));
	Value* int_ = evaluate_type(env, symbol("int"));
	Value* composite = evaluate_typefun_form(env, list(operator_, int_));
	debug_print(composite);
	puts("");

	return 0;
}
