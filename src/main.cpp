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
	Cell(Value const* head, Value const* tail):
		Value(cell_type), head(head), tail(tail) {}

	Value const* head;
	Value const* tail;
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

Value const* _f = new Value(0);

Value const* cons(Value const* head, Value const* tail)
{
	return new Cell(head, tail);
}

bool consp(Value const* x)
{
	return x->type == cell_type;
}

Value const* list()
{
	return 0;
}

Value const* list(Value const* x0)
{
	return cons(x0, list());
}

Value const* list(Value const* x0, Value const* x1)
{
	return cons(x0, list(x1));
}

Value const* list(Value const* x0, Value const* x1, Value const* x2)
{
	return cons(x0, list(x1, x2));
}

Value const* list(Value const* x0, Value const* x1, Value const* x2,
		Value const* x3)
{
	return cons(x0, list(x1, x2, x3));
}

#include <map>
#include <string>
Value const* symbol(char const* s)
{
	using namespace std;

	typedef map<string, Value const*> SymMap;
	static SymMap symbol_map;
	string str(s);
	SymMap::iterator pos = symbol_map.find(str);
	if (pos == symbol_map.end())
	{
		Value const* symbol = new Symbol(s);
		pos = symbol_map.insert(make_pair(str, symbol)).first;
	}

	return (*pos).second;
}

bool symbolp(Value const* x)
{
	return x->type == symbol_type;
}

Value const* car(Value const* x)
{
	assert(consp(x));
	return ((Cell*)x)->head;
}

Value const* cdr(Value const* x)
{
	assert(consp(x));
	return ((Cell*)x)->tail;
}

Value const* cadr(Value const* x)
{
	assert(consp(x));
	return car(((Cell*)x)->tail);
}

Value const* cddr(Value const* x)
{
	assert(consp(x));
	return cdr(((Cell*)x)->tail);
}

Value const* caddr(Value const* x)
{
	assert(consp(x));
	return cadr(((Cell*)x)->tail);
}

Value const* cdddr(Value const* x)
{
	assert(consp(x));
	return cddr(((Cell*)x)->tail);
}

Value const* reverse_helper(Value const* l, Value const* x)
{
	if (0 == l)
		return x;
	assert(consp(l));
	return reverse_helper(cdr(l), cons(car(l), x));
}

Value const* reverse(Value const* x)
{
	return reverse_helper(x, 0);
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

Value const* parse_module(context* c);
Value const* parse_define(context* c);
Value const* parse_statement(context* c);
Value const* parse_symbol_statement(context* c);
Value const* parse_symbol_define(context* c);
Value const* parse_form_statement(context* c);
Value const* parse_form_statement_contents(context* c);
Value const* parse_form_statement_tail(context* c);
Value const* parse_non_thunk_tail(context* c);
Value const* parse_define_tail(context* c);
Value const* parse_form_expression_tail(context* c);
Value const* parse_form_define(context* c);
Value const* parse_parameter(context* c);
Value const* parse_expression(context* c);
Value const* parse_form_expression(context* c);

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

Value const* parse_identifier(context* c)
{
	if (c->symbol == p_ident)
	{
		Value const* id = symbol(c->token);
		read_symbol(c);
		return id;
	}
	return _f;
}

//module = {define}
Value const* parse_module(context* c)
{
	Value const* defines_reverse = 0;
	while (c->symbol != p_eof)
	{
		Value const* define = parse_define(c);
		assert(_f != define);
		defines_reverse = cons(define, defines_reverse);
	}
	return cons(symbol("module"), reverse(defines_reverse));
}

//define =
//	form_define
//	| symbol_define
Value const* parse_define(context* c)
{
	Value const* define = _f;
	define = (_f == define ? parse_form_define(c) : define);
	define = (_f == define ? parse_symbol_define(c) : define);
	return define;
}

//statement =
//	form_statement
//	| symbol_statement
Value const* parse_statement(context* c)
{
	Value const* statement = _f;
	statement = (_f == statement ? parse_form_statement(c) : statement);
	statement = (_f == statement ? parse_symbol_statement(c) : statement);
	return statement;
}

//symbol_statement =
//	identifier ["=" expression]
Value const* parse_symbol_statement(context* c)
{
	Value const* identifier = parse_identifier(c);
	if (_f == identifier)
		return _f;

	if (accept(c, p_equal))
	{
		Value const* expression = parse_expression(c);
		return list(symbol("defval"), identifier, expression);
	}
	else
	{
		return identifier;
	}
}

//symbol_define =
//	identifier "=" expression
Value const* parse_symbol_define(context* c)
{
	Value const* identifier = parse_identifier(c);
	if (_f == identifier)
		return 0;

	expect(c, p_equal);

	Value const* expression = parse_expression(c);

	return list(symbol("define"), identifier, expression);
}

//form_statement =
//	"<" form_statement_contents
Value const* parse_form_statement(context* c)
{
	if (!accept(c, p_lt))
		return _f;
	return parse_form_statement_contents(c);
}

//form_statement_contents =
//	identifier form_statement_tail
//	| {statement} ">"
Value const* parse_form_statement_contents(context* c)
{
	Value const* identifier = parse_identifier(c);
	if (_f != identifier)
	{
		Value const* tail = parse_form_statement_tail(c);

		assert(_f != tail);
		Value const* field = car(tail);

		// Check whether we have parsed a new function.
		if (symbol("parameters") == car(field))
		{
			Value const* parameters = cdr(field);
			Value const* expression = cadr(tail);

			return list(symbol("defun"), identifier, parameters, expression);
		}
		// Check whether we have parsed a form.
		else if (symbol("statements") == car(field))
		{
			Value const* statements = cdr(field);
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
		Value const* head = parse_expression(c);
		assert(head);

		Value const* statements_reverse = 0;
		while (c->symbol != p_gt)
		{
			Value const* statement = parse_statement(c);
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
Value const* parse_form_statement_tail(context* c)
{
	if (c->symbol == p_gt)
	{
		Value const* expression = parse_define_tail(c);
		return list(list(symbol("parameters")), expression);
	}
	else
	{
		Value const* identifier = parse_identifier(c);
		if (_f != identifier)
		{
			Value const* tail = parse_non_thunk_tail(c);
			if (symbol("prototype") == car(tail))
			{
				Value const* initial_type = cadr(tail);
				Value const* expression = caddr(tail);
				Value const* subsequent_parameters = cdddr(tail);

				return list(
						cons(
							symbol("parameters"),
							cons(list(initial_type, identifier),
								subsequent_parameters)),
						expression);
			}
			else if (symbol("form") == car(tail))
			{
				Value const* statements = cdr(tail);
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
			Value const* statements_reverse = 0;
			while (c->symbol != p_gt)
			{
				Value const* statement = parse_statement(c);
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
Value const* parse_non_thunk_tail(context* c)
{
	if (accept(c, p_colon))
	{
		Value const* initial_type = parse_expression(c);

		Value const* subsequent_reverse = 0;
		while (c->symbol != p_gt)
		{
			Value const* parameter = parse_parameter(c);
			assert(_f != parameter);
			subsequent_reverse = cons(parameter, subsequent_reverse);
		}
		Value const* subsequent_parameters = reverse(subsequent_reverse);

		expect(c, p_gt);

		Value const* expression = parse_define_tail(c);
		return cons(symbol("prototype"),
				cons(initial_type,
					cons(expression,
						subsequent_parameters)));
	}
	else
	{
		Value const* subsequent_reverse = 0;
		while (c->symbol != p_gt)
		{
			Value const* statement = parse_statement(c);
			assert(_f != statement);
			subsequent_reverse = cons(statement, subsequent_reverse);
		}
		Value const* subsequent_arguments = reverse(subsequent_reverse);

		expect(c, p_gt);

		return cons(symbol("form"), subsequent_arguments);
	}
}

//define_tail =
//	"=" expression
Value const* parse_define_tail(context* c)
{
	if (!accept(c, p_equal))
		return _f;

	Value const* expression = parse_expression(c);

	assert(_f != expression);

	return expression;
}

//form_expression_tail =
//	{statement} ">"
Value const* parse_form_expression_tail(
		context* c)
{
	Value const* statements_reverse = 0;
	while (c->symbol != p_gt)
	{
		Value const* statement = parse_statement(c);
		assert(_f != statement);
		statements_reverse = cons(statement, statements_reverse);
	}

	expect(c, p_gt);

	return reverse(statements_reverse);
}

//form_define =
//	"<" expression {parameter} ">" = expression
Value const* parse_form_define(context* c)
{
	if (!accept(c, p_lt))
		return _f;

	Value const* name = parse_expression(c);
	assert(_f != name);

	Value const* parameters_reverse = 0;
	while (c->symbol != p_gt)
	{
		Value const* parameter = parse_parameter(c);
		assert(_f != parameter);
		parameters_reverse = cons(parameter, parameters_reverse);
	}
	Value const* parameters = reverse(parameters_reverse);

	expect(c, p_gt);
	expect(c, p_equal);

	Value const* expression = parse_expression(c);

	return list(symbol("defun"), name, parameters, expression);
}

//parameter =
//	identifier ":" identifier
Value const* parse_parameter(context* c)
{
	Value const* name = parse_identifier(c);
	assert(_f != name);

	expect(c, p_colon);

	Value const* type = parse_expression(c);
	assert(_f != type);

	return list(type, name);
}

//expression =
//	identifier
//	| form_expression
Value const* parse_expression(context* c)
{
	Value const* identifier = parse_identifier(c);
	if (_f != identifier)
		return identifier;
	return parse_form_expression(c);
}

//form_expression =
//	"<" expression form_expression_tail
Value const* parse_form_expression(context* c)
{
	if (!accept(c, p_lt))
		return _f;

	Value const* head = parse_expression(c);
	assert(_f != head);

	Value const* tail = parse_form_expression_tail(c);
	assert(_f != tail);

	return cons(head, tail);
}

Value const* parse_file(char const* filename)
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

void debug_print(Value const* x)
{
	if (0 == x)
	{
		printf("()");
	}
	else if (consp(x))
	{
		printf("(");
		for (Value const* l = x; l; l = cdr(l))
		{
			if (consp(l))
			{
				debug_print(car(l));
				if (cdr(l))
					printf(" ");
			}
			else
			{
				printf(" . ");
				debug_print(l);
				break;
			}
		}
		printf(")");
	}
	else if (symbolp(x))
	{
		char const* str = ((Symbol*)x)->sym;
		fputs(str, stdout);
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	Value const* module_ast = parse_file("test/test.jest");
	debug_print(module_ast);
	puts("");

	return 0;
}
