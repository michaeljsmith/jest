#include <set>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <tr1/memory>

struct AssertRaiser
{
	AssertRaiser(bool condition, char const* file, int line, char const* msg)
	{
		if (!condition)
		{
			fprintf(stderr, "%s(%d): Assertion failed: %s\n", file, line, msg);
			exit(1);
		}
	}
};
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define ASSERT_OBJ_NAME_DETAIL(a, b) a##b
#define ASSERT_OBJ_NAME(a, b) ASSERT_OBJ_NAME_DETAIL(a, b)
#define ASSERT(x) AssertRaiser ASSERT_OBJ_NAME(assertObj, __LINE__)((x), __FILE__, __LINE__, #x);

struct Cell
{
	Cell(void* head, void* tail): head(head), tail(tail) {}
	void* head;
	void* tail;
};

Cell* make_cell(void* head, void* tail)
{
	using namespace std;

	typedef map<std::pair<void*, void*>, Cell*> CellMap;
	static CellMap cell_map;
	CellMap::iterator pos = cell_map.find(make_pair(head, tail));
	if (pos == cell_map.end())
	{
		Cell* cell = new Cell(head, tail);
		pos = cell_map.insert(make_pair(make_pair(head, tail), cell)).first;
	}

	return (*pos).second;
}

char* intern(char const* s)
{
	using namespace std;

	typedef set<string> SymMap;
	static SymMap symbol_map;
	string str(s);
	SymMap::iterator pos = symbol_map.find(str);
	if (pos == symbol_map.end())
	{
		pos = symbol_map.insert(str).first;
	}

	return (char*)(*pos).c_str();
}

struct ExpressionNode;
struct Value;
struct Expression
{
	typedef std::tr1::shared_ptr<ExpressionNode const> Ptr;

	Expression(Ptr ptr): ptr(ptr)
	{
		if (ptr)
			ASSERT(!is_leaf() || !is_lambda());
	}
	Expression(Value value);

	bool is_leaf() const;
	bool is_lambda() const;

	Ptr ptr;
};

struct Value
{
	Value(Cell* cell): cell(cell) {}
	Value(Expression expr);

	Cell* cell;
};

bool operator==(Value const& l, Value const& r)
{
	return l.cell == r.cell;
}

bool operator!=(Value const& l, Value const& r)
{
	return l.cell != r.cell;
}

typedef Value (* Code)(Value context, void* data, Value argument);

typedef std::tr1::shared_ptr<Value const> ValuePtr;

struct ExpressionNode
{
	ExpressionNode(Expression f, Expression a): function(f), argument(a) {}

	ExpressionNode():
		function(std::tr1::shared_ptr<ExpressionNode const>()),
		argument(std::tr1::shared_ptr<ExpressionNode const>()),
		value() {}

	static Expression make(Expression x0, Expression x1)
	{
		std::tr1::shared_ptr<ExpressionNode const> ptr(new ExpressionNode(x0, x1));
		return ptr;
	}

	static Expression make(Value v)
	{
		std::tr1::shared_ptr<ExpressionNode> ptr(new ExpressionNode());
		ValuePtr value_ptr(new Value(v));
		ptr->value = value_ptr;
		return Expression(ptr);
	}

	static Expression make_lambda(Value v, Expression expr)
	{
		std::tr1::shared_ptr<ExpressionNode> ptr(new ExpressionNode());
		ptr->function = expr;
		ValuePtr lambda_arg(new Value(v));
		ptr->lambda_arg = lambda_arg;
		return Expression(ptr);
	}

	operator Value() const;

	Expression function;
	Expression argument;
	ValuePtr value;
	ValuePtr lambda_arg;
};

Expression::Expression(Value value)
{
	*this = ExpressionNode::make(value);
	ASSERT(!is_leaf() || !is_lambda());
}

bool Expression::is_leaf() const
{
	ASSERT(ptr);
	return ptr->value;
}

bool Expression::is_lambda() const
{
	ASSERT(ptr);
	return ptr->lambda_arg;
}

Code* box_function(Code code)
{
	using namespace std;

	typedef map<Code, Code*> CellMap;
	static CellMap cell_map;
	CellMap::iterator pos = cell_map.find(code);
	if (pos == cell_map.end())
	{
		Code* box = new Code[1];
		box[0] = code;
		pos = cell_map.insert(make_pair(code, box)).first;
	}

	return (*pos).second;
}

Value uapply(Value context, Value function, Value argument)
{
	Code code = *(Code*)(function.cell->head);
	void* data = function.cell->tail;
	return code(context, data, argument);
}

Value make_combinator(Code code, void* data)
{
	return Value(make_cell(box_function(code), data));
}

//ufail
Value code_fail1(Value /*context*/, void* /*data*/, Value argument)
{
	fprintf(stderr, "ufail\n");
	ASSERT(0);
	exit(1);
	return argument;
}

Value code_fail(Value /*context*/, void* /*data*/, Value argument)
{
	return make_combinator(code_fail1, argument.cell);
}
Value ufail = make_combinator(code_fail, 0);

//ucompose
Value code_compose2(Value context, void* data, Value argument)
{
	Cell* fs = (Cell*)data;
	ASSERT(fs);
	Cell* f0 = (Cell*)(fs->head);
	ASSERT(f0);
	Cell* f1 = (Cell*)(fs->tail);
	ASSERT(f1);
	return uapply(context, Value(f0), uapply(context, Value(f1), argument));
}

Value code_compose1(Value /*context*/, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	ASSERT(f0);
	return make_combinator(code_compose2, make_cell(f0, argument.cell));
}

Value code_compose(Value /*context*/, void* /*data*/, Value argument)
{
	return make_combinator(code_compose1, argument.cell);
}

Value ucompose = make_combinator(code_compose, 0);

//uflip
Value code_flip2(Value context, void* data, Value argument)
{
	Cell* fs = (Cell*)data;
	ASSERT(fs);
	Cell* f0 = (Cell*)(fs->head);
	ASSERT(f0);
	Cell* f1 = (Cell*)(fs->tail);
	ASSERT(f1);
	return uapply(context, uapply(context, Value(f0), argument), Value(f1));
}

Value code_flip1(Value /*context*/, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	ASSERT(f0);
	return make_combinator(code_flip2, make_cell(f0, argument.cell));
}

Value code_flip(Value /*context*/, void* /*data*/, Value argument)
{
	return make_combinator(code_flip1, argument.cell);
}

Value uflip = make_combinator(code_flip, 0);

//uconstant
Value code_constant1(Value /*context*/, void* data, Value /*argument*/)
{
	Cell* f0 = (Cell*)data;
	ASSERT(f0);
	return f0;
}

Value code_constant(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(argument.cell);
	return make_combinator(code_constant1, argument.cell);
}

Value uconstant = make_combinator(code_constant, 0);

//uduplicate
Value code_duplicate1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	ASSERT(f0);
	return uapply(context, uapply(context, Value(f0), argument), argument);
}

Value code_duplicate(Value /*context*/, void* /*data*/, Value argument)
{
	return make_combinator(code_duplicate1, argument.cell);
}

Value uduplicate = make_combinator(code_duplicate, 0);

// DSL
Value evaluate(Value context, Expression expr);

	Value::Value(Expression expr)
: cell(evaluate(Value(0), expr).cell)
{
}

//operator*
Expression operator*(Value const& x0, Value const& x1)
{
	return ExpressionNode::make(ExpressionNode::make(x0), ExpressionNode::make(x1));
}

Expression operator*(Expression x0, Expression x1)
{
	return ExpressionNode::make(x0, x1);
}

Expression operator*(Expression x0, Value const& x1)
{
	return ExpressionNode::make(x0, ExpressionNode::make(x1));
}

Expression operator*(Value const& x0, Expression x1)
{
	return ExpressionNode::make(ExpressionNode::make(x0), x1);
}

//operator+
Expression operator+(Value const& x0, Value const& x1)
{
	return ucompose * x0 * x1;
}

Expression operator+(Expression x0, Expression x1)
{
	return ucompose * x0 * x1;
}

Expression operator+(Expression x0, Value const& x1)
{
	return ucompose * x0 * x1;
}

Expression operator+(Value const& x0, Expression x1)
{
	return ucompose * x0 * x1;
}

ASSERT((uflip + uconstant) * ufail == uflip * (uconstant * ufail));
ASSERT(ucompose == uflip * uconstant * ufail * ucompose);
ASSERT(ucompose == uconstant * ucompose * ufail);
ASSERT(uduplicate * ucompose * ufail == ufail + ufail);

//uidentity
Value uidentity = uduplicate * uconstant;
ASSERT(ucompose == uidentity * ucompose);

//utrue
Value utrue = uconstant;
ASSERT(utrue * uduplicate * ucompose == uduplicate);

//ufalse
Value ufalse = uconstant * uidentity;
ASSERT(ufalse * uduplicate * ucompose == ucompose);

Value boolean(bool x)
{
	return x ? utrue : ufalse;
}

//ucode
Value code_code(Value /*context*/, void* /*data*/, Value /*argument*/)
{
	ASSERT(0);
	return ufail;
}

Value make_code(Code* code)
{
	return make_combinator(code_code, code);
}

//ucodeof
Value code_codeof(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(argument.cell);
	Code* code = (Code*)argument.cell->head;
	ASSERT(code);
	return make_code(code);
}

Value ucodeof = make_combinator(code_codeof, 0);
ASSERT(ucodeof * ufail == ucodeof * ufail);
ASSERT(ucodeof * ucompose != ucodeof * ufail);

//ucons
Value ucons = uflip + (uflip * uidentity);
ASSERT(uidentity * ucompose * ufail == ucompose * ufail);
ASSERT(uflip * uidentity * ucompose * uconstant == uconstant * ucompose);
ASSERT(ucons * ufail * ucompose * utrue == ufail);
ASSERT(ucons * ufail * ucompose * ufalse == ucompose);

//unot
Value unot = ucons * ufalse * utrue;
ASSERT(unot * ufalse == utrue);
ASSERT(unot * utrue == ufalse);

//uidentical
Value code_identical1(Value /*context*/, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	ASSERT(f0);
	return boolean(f0 == argument.cell);
}

Value code_identical(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(argument.cell);
	return make_combinator(code_identical1, argument.cell);
}

Value uidentical = make_combinator(code_identical, 0);
ASSERT(uidentical * ufail * ufail == utrue);
ASSERT(uidentical * ufail * ucompose == ufalse);

//ucomposep
namespace detail
{
	Value compose_sample0 = ufail + ufail;
	Value compose_code = ucodeof * compose_sample0;
	Value compose_sample1 = utrue + ufalse;
}
Value ucomposep = uidentical * detail::compose_code + ucodeof;
ASSERT(ufalse == ucomposep * ufail);
ASSERT(utrue == ucomposep * detail::compose_sample1);

//uconsp
//TODO: Implement properly.
namespace detail
{
	Value flip2_code = ucodeof * (uflip * ufail * utrue);
}
Value uconsp = uidentical * detail::flip2_code + ucodeof;
ASSERT(uconsp * (ucons * ufail * ufalse) == utrue);
ASSERT(uconsp * ucons == ufalse);
ASSERT(uconsp * uidentity == ufalse);
ASSERT(uconsp * (ucons * utrue) == ufalse);
ASSERT(uconsp * (ucons * (ucons * utrue * utrue) * ufalse) == utrue);

//ucar
Value ucar = uflip * uidentity * utrue;
ASSERT(ucar * (ucons * ufail * ucompose) == ufail);

//ucdr
Value ucdr = uflip * uidentity * ufalse;
ASSERT(ucdr * (ucons * ufail * ucompose) == ucompose);

//usymbol
Value code_symbol(Value /*context*/, void* /*data*/, Value /*argument*/)
{
	ASSERT(0);
	return ufail;
}

Value usymbol(char const* str)
{
	return make_combinator(code_fail, intern(str));
}

//usymbolp
Value code_symbolp(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(argument.cell);
	Code* box = (Code*)argument.cell->head;
	return boolean(*box == code_symbol);
}

Value usymbolp = make_combinator(code_symbolp, 0);

//ucurry
//Value ucurry = ulambda(f) {ulambda(x) {ulambda(y) {f * (ucons * x * y)}}}
//            = ulambda(f) {ulambda(x) {ulambda(y) {(f + (ucons * x)) * y)}}}
ASSERT(ucompose * (ucons * ufalse * utrue) == (ucompose + (ucons * ufalse)) * utrue);
ASSERT(uflip * (ucons * ufail * ucompose) == (uflip + (ucons * ufail)) * ucompose);
//            = labmda(f) {ulambda(x) {f + ucons * x}}
//            = ulambda(f) {ulambda(x) {ucompose * f * (ucons * x)}}
ASSERT(ucompose + ucons * utrue == ucompose * ucompose * (ucons * utrue));
ASSERT(uflip + ucons * ufail == ucompose * uflip * (ucons * ufail));
//            = ulambda(f) {ulambda(x) {ucompose * (ucompose * f) * ucons * x}}
ASSERT(ucompose * ucompose * (ucons * utrue) == ucompose * (ucompose * ucompose) * ucons * utrue);
ASSERT(ucompose * uflip * (ucons * ufail) == ucompose * (ucompose * uflip) * ucons * ufail);
//            = ulambda(f) {ucompose * (ucompose * f) * ucons}
//            = ulambda(f) {ucompose * ucompose * ucompose * f * ucons}
ASSERT(ucompose * (ucompose * ucompose) * ucons == ucompose * ucompose * ucompose * ucompose * ucons);
ASSERT(ucompose * (ucompose * uflip) * ucons == ucompose * ucompose * ucompose * uflip * ucons);
//            = ulambda(f) {uflip * (ucompose * ucompose * ucompose) * ucons * f}
ASSERT(ucompose * ucompose * ucompose * ucompose * ucons == uflip * (ucompose * ucompose * ucompose) * ucons * ucompose);
ASSERT(ucompose * ucompose * ucompose * uflip * ucons == uflip * (ucompose * ucompose * ucompose) * ucons * uflip);
//            = uflip * (ucompose * ucompose * ucompose) * ucons
Value ucurry = uflip * (ucompose * ucompose * ucompose) * ucons;
ASSERT(ucurry * ucar * utrue * ufalse == utrue);
ASSERT(ucurry * ucdr * utrue * ufalse == ufalse);

//uuncurry
namespace detail
{
	Value pair_sample0 = ucons * ufalse * utrue;
	Value pair_sample1 = ucons * utrue * usymbolp;
}
//Value uuncurry = ulambda(f) {ulambda(p) {f * (ucar * p) * (ucdr * p)}}
//              = ulambda(f) {ulambda(p) {ucompose * f * ucar * p * (ucdr * p)}}
ASSERT(uflip * (ucar * detail::pair_sample0) * (ucdr * detail::pair_sample0) == ucompose * uflip * ucar * detail::pair_sample0 * (ucdr * detail::pair_sample0));
ASSERT(uduplicate * (ucar * detail::pair_sample1) * (ucdr * detail::pair_sample1) == ucompose * uduplicate * ucar * detail::pair_sample1 * (ucdr * detail::pair_sample1));
//              = ulambda(f) {ulambda(p) {uflip * (ucompose * f * ucar) * (ucdr * p) * p}}
ASSERT(ucompose * uflip * ucar * detail::pair_sample0 * (ucdr * detail::pair_sample0) == uflip * (ucompose * uflip * ucar) * (ucdr * detail::pair_sample0) * detail::pair_sample0);
ASSERT(ucompose * uduplicate * ucar * detail::pair_sample1 * (ucdr * detail::pair_sample1) == uflip * (ucompose * uduplicate * ucar) * (ucdr * detail::pair_sample1) * detail::pair_sample1);
//              = ulambda(f) {ulambda(p) {ucompose * (uflip * (ucompose * f * ucar)) * ucdr * p * p}}
ASSERT(uflip * (ucompose * uflip * ucar) * (ucdr * detail::pair_sample0) * detail::pair_sample0 == ucompose * (uflip * (ucompose * uflip * ucar)) * ucdr * detail::pair_sample0 * detail::pair_sample0);
ASSERT(uflip * (ucompose * uduplicate * ucar) * (ucdr * detail::pair_sample1) * detail::pair_sample1 == ucompose * (uflip * (ucompose * uduplicate * ucar)) * ucdr * detail::pair_sample1 * detail::pair_sample1);
//              = ulambda(f) {ulambda(p) {uduplicate * (ucompose * (uflip * (ucompose * f * ucar)) * ucdr) * p}}
ASSERT(ucompose * (uflip * (ucompose * uflip * ucar)) * ucdr * detail::pair_sample0 * detail::pair_sample0 == uduplicate * (ucompose * (uflip * (ucompose * uflip * ucar)) * ucdr) * detail::pair_sample0);
ASSERT(ucompose * (uflip * (ucompose * uduplicate * ucar)) * ucdr * detail::pair_sample1 * detail::pair_sample1 == uduplicate * (ucompose * (uflip * (ucompose * uduplicate * ucar)) * ucdr) * detail::pair_sample1);
//
//              = ulambda(f) {uduplicate * (ucompose * (uflip * (ucompose * f * ucar)) * ucdr)}
//              = ulambda(f) {uduplicate * (ucompose * (uflip * (uflip * ucompose * ucar * f)) * ucdr)}
ASSERT(uduplicate * (ucompose * (uflip * (ucompose * uflip * ucar)) * ucdr) == uduplicate * (ucompose * (uflip * (uflip * ucompose * ucar * uflip)) * ucdr));
ASSERT(uduplicate * (ucompose * (uflip * (ucompose * uduplicate * ucar)) * ucdr) == uduplicate * (ucompose * (uflip * (uflip * ucompose * ucar * uduplicate)) * ucdr));
//              = ulambda(f) {uduplicate * (ucompose * (ucompose * uflip * (uflip * ucompose * ucar) * f) * ucdr)}
ASSERT(uduplicate * (ucompose * (uflip * (uflip * ucompose * ucar * uflip)) * ucdr) == uduplicate * (ucompose * (ucompose * uflip * (uflip * ucompose * ucar) * uflip) * ucdr));
ASSERT(uduplicate * (ucompose * (uflip * (uflip * ucompose * ucar * uduplicate)) * ucdr) == uduplicate * (ucompose * (ucompose * uflip * (uflip * ucompose * ucar) * uduplicate) * ucdr));
//              = ulambda(f) {uduplicate * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar)) * f * ucdr)}
ASSERT(uduplicate * (ucompose * (ucompose * uflip * (uflip * ucompose * ucar) * uflip) * ucdr) == uduplicate * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar)) * uflip * ucdr));
ASSERT(uduplicate * (ucompose * (ucompose * uflip * (uflip * ucompose * ucar) * uduplicate) * ucdr) == uduplicate * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar)) * uduplicate * ucdr));
//              = ulambda(f) {uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr * f)}
ASSERT(uduplicate * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar)) * uflip * ucdr) == uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr * uflip));
ASSERT(uduplicate * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar)) * uduplicate * ucdr) == uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr * uduplicate));
//              = ulambda(f) {ucompose * uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr) * f}
ASSERT(uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr * uflip) == ucompose * uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr) * uflip);
ASSERT(uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr * uduplicate) == ucompose * uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr) * uduplicate);

Value uuncurry = ucompose * uduplicate * (uflip * (ucompose * ucompose * (ucompose * uflip * (uflip * ucompose * ucar))) * ucdr);

ASSERT(uuncurry * utrue * (ucons * utrue * ufalse) == utrue);
ASSERT(uuncurry * ufalse * (ucons * utrue * ufalse) == ufalse);

//uarg
Value code_arg(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(0);
	exit(1);
	return argument;
}

Expression uarg(char const* sym)
{
	return ExpressionNode::make(make_combinator(code_arg, usymbol(sym).cell));
}

//usubstitute
//Sxyz = xz(yz)
Value usubstitute = ucompose * (ucompose * uduplicate) * (ucompose * ucompose * uflip);
ASSERT(usubstitute * ucompose * uflip * uduplicate == ucompose * uduplicate * (uflip * uduplicate));
ASSERT(usubstitute * uflip * ucompose * uconstant == uflip * uconstant * (ucompose * uconstant));

//ulambda
Expression ulambda(char const* param, Expression expr)
{
	return ExpressionNode::make_lambda(uarg(param), expr);
}

//ASSERT(uidentity == ulambda("x", uarg("x")));
ASSERT(ulambda("x", ucompose) * ufail == ucompose);
ASSERT(ulambda("x", uarg("x")) == uidentity);
ASSERT(ulambda("x", ucompose * ucompose) * ufail == ucompose * ucompose);
ASSERT(ulambda("x", uflip * uarg("x")) * ucons * utrue * ufalse == ucons * ufalse * utrue);
ASSERT(ulambda("x", ucompose * uarg("x") * (uflip * uarg("x"))) * ufail == usubstitute * ucompose * uflip * ufail);
ASSERT(ulambda("x", ulambda("y", uarg("x") * uarg("y"))) * uflip * ucompose == uflip * ucompose);
ASSERT(ulambda("x", ulambda("y", uarg("x") * uarg("y") * (uarg("y") * uarg("x")))) * uflip * ucompose == uflip * ucompose * (ucompose * uflip));

//ufix
//(define applicative-order-y
//  (ulambda (f)
//    ((ulambda (x) 
//       (f (ulambda (uarg) ((x x) uarg))))
//     (ulambda (x) 
//       (f (ulambda (uarg) ((x x) uarg)))))))
Value ufix = ulambda("f",
		ulambda("x", uarg("f") * ulambda("y", (uarg("x") * uarg("x")) * uarg("y"))) *
		ulambda("x", uarg("f") * ulambda("y", (uarg("x") * uarg("x")) * uarg("y"))));

namespace detail
{
	Value inc = ucons * ucons;

	Value recurse_sample_gen = ulambda("t",
			ulambda("f",
				ulambda("l",
					((ucons *
					  (uconstant * uarg("l")) *
					  uarg("f")) *
					 (uidentical * uarg("t") * uarg("l"))) *
					(inc * uarg("l")))));

	Value initial = (ucons * utrue * ufalse);
	Value recurse_sample0 = recurse_sample_gen * initial;

	Value second = (inc * initial);
	Value recurse_sample1 = recurse_sample_gen * second;

	Value third = (inc * second);

	Value fix_sample = ufix * (recurse_sample_gen * third);
}

ASSERT(detail::recurse_sample0 * ufail * detail::initial == detail::initial);
ASSERT(detail::recurse_sample1 * uidentity * detail::initial == detail::second);
ASSERT(detail::fix_sample * detail::initial == detail::third);

//ucheck
Value ucheck = ulambda("m", ulambda("c",
			(ucons *
			 uidentity *
			 (ufail * uarg("m"))) *
			uarg("c") * utrue));

ASSERT(uidentity * (ucheck * usymbol("test_message") * utrue) == utrue);

//uif
Value uif = ulambda("c", ulambda("t", ulambda("f",
				ucons * uarg("t") * uarg("f") * uarg("c"))));
ASSERT(uif * utrue * uflip * uconstant == uflip);
ASSERT(uif * ufalse * uflip * uconstant == uconstant);

//uconditional
Value udefault = usymbol("default");
Value uconditional_recurse_step = ulambda("uconditional_recurse",
		ulambda("h", ulambda("c", ulambda("v",
					(uif * (uidentical * udefault * uarg("c")) *
					 (uarg("h") * uarg("v")) *
					 (uarg("uconditional_recurse") *
					  (ulambda("d",
							   uarg("h") * (uif * uarg("c") * uarg("v") * uarg("d"))))))))));
ASSERT(uconditional_recurse_step * uidentity * uidentity * udefault * uduplicate == uduplicate);
Value uconditional_recurse = ufix * uconditional_recurse_step;
Value uconditional = uconditional_recurse * uidentity;
ASSERT(uconditional * udefault * uduplicate == uduplicate);
ASSERT(uconditional * utrue * uflip * udefault * uduplicate == uflip);
ASSERT(uconditional * ufalse * uflip * udefault * uduplicate == uduplicate);
ASSERT(uconditional * ufalse * ucons * ufalse * uflip * udefault * uconstant == uconstant);
ASSERT(uconditional * utrue * ucons * ufalse * uflip * udefault * uconstant == ucons);

////umodule
//#define umodule(n, cs) (ufix * ulambda((n), ulambda("_symbol", (cs) * ufail)))
//
////udefine
//#define udefine(s, x) (uidentical * uarg("s") * uarg("_symbol")) * ulambda("dummy", (x))
//
//ASSERT(umodule("test", udefine("foo", ufail)) * usymbol("foo") == ufail);
//ASSERT(umodule("test",
//			udefine("foo", ufail) *
//			udefine("bar", uflip)) * usymbol("bar") == uflip);

////uTag
//Value uTag = usymbol("uTag");
//
////uis
//Value uis = uidentity;
//
////uMorphism
//Value uMorphism = usymbol("uMorphism");
//
////umaps
//Value umaps = ucons;
//
////uProduct
//Value uProduct = usymbol("uProduct");
//
////uboth
//Value uboth = ucons;
//
////uSum
//Value uSum = usymbol("uSum");
//
////ueither
//Value ueither = ucons;
//
////uSymbol
//Value uSymbol = usymbol("Symbol");

////Type
//Value make_Type_recurse = ulambda("make_Type", ulambda("member",
//			(uconditional *
//
//			 (uidentical * usymbol("Type") * uarg("member")) *
//			 ulambda("selector",
//				 (uarg("selector") *
//				  (uarg("make_Type") * usymbol("Type")) *
//				  (ueither *
//				   (uarg("make_Type") * usymbol("Symbol")) *
//				   (uarg("make_Type") * usymbol("Sum"))))) *
//
////			 (uidentical * usymbol("Type") * uarg("member")) *
////			 (uarg("make_Type") * usymbol("Sum") *
////			  (uarg("make_Type") * usymbol("Symbol")) *
////			  (uarg("make_Type") * usymbol("Sum") *
////			   (uarg("make_Type") * usymbol("Type")) *
////			   (uarg("make_Type") * usymbol("Type")))) *
//
//			 (uidentical * usymbol("Symbol") * uarg("member")) *
//			 ulambda("selector",
//				 (uarg("selector") *
//				  (uarg("make_Type") * usymbol("Type")) *
//				  (ucons * utrue * ufail))) *
//
//			 (uidentical * usymbol("Sum") * uarg("member")) *
//			 ulambda("first", ulambda("second",
//					 ulambda("selector",
//						 (uarg("selector") *
//						  (uarg("make_Type") * usymbol("Type")) *
//						  (ucons * ufalse * (ueither * uarg("first") * uarg("second"))))))) *
//
//			 udefault * ufail
//			 )));
//Value make_Type = ufix * make_Type_recurse;

//urapply
Value urapply = uflip * uidentity;
ASSERT(urapply * (ucons * utrue * ufalse) * ucdr == ufalse);

//uswitch
Value uswitch = ucons;

//umapswitch
Value umapswitch = ulambda("e", ulambda("f", ulambda("s",
				((ucar * uarg("e")) *
				 (uarg("f") * (ucdr * uarg("e"))) *
				 (uarg("s") * (ucdr * uarg("e")))))));
ASSERT(umapswitch * (uswitch * utrue * usymbol("hello")) * ulambda("x", usymbol("first")) * ulambda("x", usymbol("second")) == usymbol("first"));
ASSERT(umapswitch * (uswitch * ufalse * usymbol("chicken")) * ulambda("x", usymbol("first")) * ulambda("x", uarg("x")) == usymbol("chicken"));

//uvoid
Value uvoid = ufail;

//type = either
//			either
//				either
//					Symbol
//					Identity
//				either
//					Morphism
//					Product
//			either
//				either
//					Sum
//					fail
//				either
//					fail
//					fail

//uSymbol
Value uSymbol = (uswitch * utrue * (uswitch * utrue * (uswitch * utrue * uvoid)));

//uSymbolp
Value uSymbolp = ulambda("t",
		(umapswitch * uarg("t") *
		 ulambda("t0",
			 (umapswitch * uarg("t0") *
			  ulambda("t1",
				  (umapswitch * uarg("t1") *
				   (uconstant * utrue) *
				   (uconstant * ufalse))) *
			  (uconstant * ufalse))) *
		 (uconstant * ufalse)));
ASSERT(uSymbolp * uSymbol == utrue);

//uIdentity
Value uIdentity = ulambda("x",
		(uswitch * utrue * (uswitch * utrue * (uswitch * ufalse * uarg("x")))));
ASSERT(uSymbolp * (uIdentity * usymbol("y")) == ufalse);

//uIdentityp
Value uIdentityp = ulambda("t",
		(umapswitch * uarg("t") *
		 ulambda("t0",
			 (umapswitch * uarg("t0") *
			  ulambda("t1",
				  (umapswitch * uarg("t1") *
				   (uconstant * ufalse) *
				   (uconstant * utrue))) *
			  (uconstant * ufalse))) *
		 (uconstant * ufalse)));
ASSERT(uIdentityp * (uIdentity * usymbol("y")) == utrue);
ASSERT(uIdentityp * uSymbol == ufalse);

//uMorphism
Value uMorphism = ulambda("t0", ulambda("t1",
			(uswitch * utrue * (uswitch * ufalse * (uswitch * utrue * (ucons * uarg("t0") * uarg("t0")))))));
ASSERT(uSymbolp * (uMorphism * usymbol("y") * usymbol("z")) == ufalse);
ASSERT(uIdentityp * (uMorphism * usymbol("y") * usymbol("z")) == ufalse);

Value types = ufix * ulambda("types", ulambda("member",
			urapply * usymbol("dummy") *

			((uidentical * usymbol("Type") * uarg("member")) *
			 ulambda("dummy", (ufail * usymbol("hello"))) *

			 ((uidentical * usymbol("Symbol") * uarg("member")) *
			  ulambda("dummy", usymbol("shouldn't run")) *

			  ((uidentical * usymbol("uIdentity") * uarg("member")) *
			   ulambda("dummy",
				   ulambda("x", uarg("x"))) *

			   ((uidentical * usymbol("Morphism") * uarg("member")) *
				ulambda("dummy", (ufail * usymbol("hello"))) *

				((uidentical * usymbol("Product") * uarg("member")) *
				 ulambda("dummy", (ufail * usymbol("hello"))) *

				 ((uidentical * usymbol("Sum") * uarg("member")) *
				  ulambda("dummy", (ufail * usymbol("hello"))) *

				  ufail))))))));

Value Type = types * usymbol("Type");
Value Symbol = types * usymbol("Symbol");
Value Identity = types * usymbol("uIdentity");
Value Morphism = types * usymbol("Morphism");
Value Product = types * usymbol("Product");
Value Sum = types * usymbol("Sum");

//Value Type = types * usymbol("Type");
//ASSERT(ucdr * Type == ucdr * (ucar * Type));
//ASSERT(ucdr * (ucar * (ucar * Type)) == ucdr * Type);
//
//Value Symbol = types * usymbol("Symbol");
//ASSERT(ucdr * (ucar * Symbol) == ucdr * (ucar * (ucar * Symbol)));
//
//Value uEither = types * usymbol("Sum");
//ASSERT(ucdr * (ucar * (uEither * Symbol * Symbol)) == ucdr * (ucar * (ucar * (uEither * Symbol * Symbol))));
//
//Value uisSymbol = ulambda("x",
//		ucar * (ucdr * uarg("x")));
//ASSERT(uisSymbol * Symbol == utrue);
//ASSERT(uisSymbol * (uEither * Symbol * Symbol) == ufalse);
//
//Value uisEither = ulambda("x",
//		unot * (ucar * (ucdr * uarg("x"))));
//ASSERT(uisEither * (uEither * Symbol * Symbol) == utrue);
//ASSERT(uisEither * Symbol == ufalse);

//tapply
Value tapply = ulambda("f", ulambda("x",
			ulambda("p",
				(ucons *
				 (ucar * uarg("p")) *
				 ((ucdr * uarg("p")) * (ucdr * uarg("x"))))) *
			(ucdr * uarg("f") * (ucar * uarg("x")))));

//tcompose
//Value tcompose = ulambda("f", ulambda("g",
//			(ucons *
//			 (compose_type * (ucar * uarg("f")) * (ucar * uarg("g"))) *
//			 (ucompose

//tflip
//Value tflip = ulambda("f",
//		(ucons *
//		 (flip_type * (ucar * uarg("f"))) *
//		 (uflip * (ucdr * uarg("f")))));

//tduplicate
//Value tduplicate = ulambda("f",
//		(ucons *
//		 (duplicate_type * (ucar * uarg("f"))) *
//		 (uduplicate * (ucdr * uarg("f")))));

//tconstant
//Value tconstant = ulambda("f",
//		(ucons *
//		 (constant_type * (ucar * uarg("f"))) *
//		 (uconstant * (ucdr * uarg("f")))));

int main()
{
	return 0;
}

Expression eliminate_lambda(Expression expr)
{
	//fprintf(stderr, "eliminate_lambda\n");

	//TODO: Optimize the output of this.
	if (expr.is_leaf())
	{
		ASSERT(!expr.is_lambda());
		return expr;
	}
	if (!expr.is_lambda())
		return ExpressionNode::make(eliminate_lambda(expr.ptr->function), eliminate_lambda(expr.ptr->argument));
	Value param = *expr.ptr->lambda_arg;
	Expression subexpr = expr.ptr->function;
	if (subexpr.is_leaf() && *subexpr.ptr->value == param)
		return ExpressionNode::make(uidentity);
	if (subexpr.is_leaf())
		return ExpressionNode::make(ExpressionNode::make(uconstant), subexpr);
	if (subexpr.is_lambda())
		return eliminate_lambda(ExpressionNode::make_lambda(param, eliminate_lambda(subexpr)));
	//T[Lx.(E1 E2)] => (S T[Lx.E1] T[Lx.E2])
	Expression subfunction = subexpr.ptr->function;
	Expression subargument = subexpr.ptr->argument;
	return ExpressionNode::make(ExpressionNode::make(ExpressionNode::make(usubstitute),
				eliminate_lambda(ExpressionNode::make_lambda(param, subfunction))),
			eliminate_lambda(ExpressionNode::make_lambda(param, subargument)));
}

Expression eliminate_lambda2(Expression expr)
{
	Expression elim = eliminate_lambda(expr);
	ASSERT(!elim.is_lambda());
	return elim;
}

Value evaluate(Value context, Expression expr)
{
	if (expr.is_lambda())
		return evaluate(context, eliminate_lambda2(expr));
	if (expr.is_leaf())
		return *expr.ptr->value;
	return uapply(context, evaluate(context, expr.ptr->function), evaluate(context, expr.ptr->argument));
}
