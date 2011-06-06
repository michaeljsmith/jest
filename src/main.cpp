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

Value apply(Value context, Value function, Value argument)
{
	Code code = *(Code*)(function.cell->head);
	void* data = function.cell->tail;
	return code(context, data, argument);
}

Value make_combinator(Code code, void* data)
{
	return Value(make_cell(box_function(code), data));
}

//fail
Value code_fail1(Value /*context*/, void* /*data*/, Value argument)
{
	fprintf(stderr, "fail\n");
	ASSERT(0);
	exit(1);
	return argument;
}

Value code_fail(Value /*context*/, void* /*data*/, Value argument)
{
	return make_combinator(code_fail1, argument.cell);
}
Value fail = make_combinator(code_fail, 0);

//compose
Value code_compose2(Value context, void* data, Value argument)
{
	Cell* fs = (Cell*)data;
	ASSERT(fs);
	Cell* f0 = (Cell*)(fs->head);
	ASSERT(f0);
	Cell* f1 = (Cell*)(fs->tail);
	ASSERT(f1);
	return apply(context, Value(f0), apply(context, Value(f1), argument));
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

Value compose = make_combinator(code_compose, 0);

//flip
Value code_flip2(Value context, void* data, Value argument)
{
	Cell* fs = (Cell*)data;
	ASSERT(fs);
	Cell* f0 = (Cell*)(fs->head);
	ASSERT(f0);
	Cell* f1 = (Cell*)(fs->tail);
	ASSERT(f1);
	return apply(context, apply(context, Value(f0), argument), Value(f1));
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

Value flip = make_combinator(code_flip, 0);

//constant
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

Value constant = make_combinator(code_constant, 0);

//duplicate
Value code_duplicate1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	ASSERT(f0);
	return apply(context, apply(context, Value(f0), argument), argument);
}

Value code_duplicate(Value /*context*/, void* /*data*/, Value argument)
{
	return make_combinator(code_duplicate1, argument.cell);
}

Value duplicate = make_combinator(code_duplicate, 0);

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
	return compose * x0 * x1;
}

Expression operator+(Expression x0, Expression x1)
{
	return compose * x0 * x1;
}

Expression operator+(Expression x0, Value const& x1)
{
	return compose * x0 * x1;
}

Expression operator+(Value const& x0, Expression x1)
{
	return compose * x0 * x1;
}

ASSERT((flip + constant) * fail == flip * (constant * fail));
ASSERT(compose == flip * constant * fail * compose);
ASSERT(compose == constant * compose * fail);
ASSERT(duplicate * compose * fail == fail + fail);

//identity
Value identity = duplicate * constant;
ASSERT(compose == identity * compose);

//true_
Value true_ = constant;
ASSERT(true_ * duplicate * compose == duplicate);

//false_
Value false_ = constant * identity;
ASSERT(false_ * duplicate * compose == compose);

Value boolean(bool x)
{
	return x ? true_ : false_;
}

//code
Value code_code(Value /*context*/, void* /*data*/, Value /*argument*/)
{
	ASSERT(0);
	return fail;
}

Value make_code(Code* code)
{
	return make_combinator(code_code, code);
}

//codeof
Value code_codeof(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(argument.cell);
	Code* code = (Code*)argument.cell->head;
	ASSERT(code);
	return make_code(code);
}

Value codeof = make_combinator(code_codeof, 0);
ASSERT(codeof * fail == codeof * fail);
ASSERT(codeof * compose != codeof * fail);

//cons
Value cons = flip + (flip * identity);
ASSERT(identity * compose * fail == compose * fail);
ASSERT(flip * identity * compose * constant == constant * compose);
ASSERT(cons * fail * compose * true_ == fail);
ASSERT(cons * fail * compose * false_ == compose);

//identical
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

Value identical = make_combinator(code_identical, 0);
ASSERT(identical * fail * fail == true_);
ASSERT(identical * fail * compose == false_);

//composep
namespace detail
{
	Value compose_sample0 = fail + fail;
	Value compose_code = codeof * compose_sample0;
	Value compose_sample1 = true_ + false_;
}
Value composep = identical * detail::compose_code + codeof;
ASSERT(false_ == composep * fail);
ASSERT(true_ == composep * detail::compose_sample1);

//consp
//TODO: Implement properly.
namespace detail
{
	Value flip2_sample0 = flip * fail;
	Value flip2_code = codeof * flip2_sample0;
}
Value consp = identical * detail::flip2_code + codeof;
ASSERT(consp * (cons * fail * false_) == true_);

//car
Value car = flip * identity * true_;
ASSERT(car * (cons * fail * compose) == fail);

//cdr
Value cdr = flip * identity * false_;
ASSERT(cdr * (cons * fail * compose) == compose);

//symbol
Value code_symbol(Value /*context*/, void* /*data*/, Value /*argument*/)
{
	ASSERT(0);
	return fail;
}

Value symbol(char const* str)
{
	return make_combinator(code_fail, intern(str));
}

//symbolp
Value code_symbolp(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(argument.cell);
	Code* box = (Code*)argument.cell->head;
	return boolean(*box == code_symbol);
}

Value symbolp = make_combinator(code_symbolp, 0);

// nil
Value nil = symbol("nil");

// nilp
Value nilp = identical * nil;

//curry
//Value curry = lambda(f) {lambda(x) {lambda(y) {f * (cons * x * y)}}}
//            = lambda(f) {lambda(x) {lambda(y) {(f + (cons * x)) * y)}}}
ASSERT(compose * (cons * false_ * true_) == (compose + (cons * false_)) * true_);
ASSERT(flip * (cons * fail * compose) == (flip + (cons * fail)) * compose);
//            = labmda(f) {lambda(x) {f + cons * x}}
//            = lambda(f) {lambda(x) {compose * f * (cons * x)}}
ASSERT(compose + cons * true_ == compose * compose * (cons * true_));
ASSERT(flip + cons * fail == compose * flip * (cons * fail));
//            = lambda(f) {lambda(x) {compose * (compose * f) * cons * x}}
ASSERT(compose * compose * (cons * true_) == compose * (compose * compose) * cons * true_);
ASSERT(compose * flip * (cons * fail) == compose * (compose * flip) * cons * fail);
//            = lambda(f) {compose * (compose * f) * cons}
//            = lambda(f) {compose * compose * compose * f * cons}
ASSERT(compose * (compose * compose) * cons == compose * compose * compose * compose * cons);
ASSERT(compose * (compose * flip) * cons == compose * compose * compose * flip * cons);
//            = lambda(f) {flip * (compose * compose * compose) * cons * f}
ASSERT(compose * compose * compose * compose * cons == flip * (compose * compose * compose) * cons * compose);
ASSERT(compose * compose * compose * flip * cons == flip * (compose * compose * compose) * cons * flip);
//            = flip * (compose * compose * compose) * cons
Value curry = flip * (compose * compose * compose) * cons;
ASSERT(curry * car * true_ * false_ == true_);
ASSERT(curry * cdr * true_ * false_ == false_);

//uncurry
namespace detail
{
	Value pair_sample0 = cons * false_ * true_;
	Value pair_sample1 = cons * true_ * symbolp;
}
//Value uncurry = lambda(f) {lambda(p) {f * (car * p) * (cdr * p)}}
//              = lambda(f) {lambda(p) {compose * f * car * p * (cdr * p)}}
ASSERT(flip * (car * detail::pair_sample0) * (cdr * detail::pair_sample0) == compose * flip * car * detail::pair_sample0 * (cdr * detail::pair_sample0));
ASSERT(duplicate * (car * detail::pair_sample1) * (cdr * detail::pair_sample1) == compose * duplicate * car * detail::pair_sample1 * (cdr * detail::pair_sample1));
//              = lambda(f) {lambda(p) {flip * (compose * f * car) * (cdr * p) * p}}
ASSERT(compose * flip * car * detail::pair_sample0 * (cdr * detail::pair_sample0) == flip * (compose * flip * car) * (cdr * detail::pair_sample0) * detail::pair_sample0);
ASSERT(compose * duplicate * car * detail::pair_sample1 * (cdr * detail::pair_sample1) == flip * (compose * duplicate * car) * (cdr * detail::pair_sample1) * detail::pair_sample1);
//              = lambda(f) {lambda(p) {compose * (flip * (compose * f * car)) * cdr * p * p}}
ASSERT(flip * (compose * flip * car) * (cdr * detail::pair_sample0) * detail::pair_sample0 == compose * (flip * (compose * flip * car)) * cdr * detail::pair_sample0 * detail::pair_sample0);
ASSERT(flip * (compose * duplicate * car) * (cdr * detail::pair_sample1) * detail::pair_sample1 == compose * (flip * (compose * duplicate * car)) * cdr * detail::pair_sample1 * detail::pair_sample1);
//              = lambda(f) {lambda(p) {duplicate * (compose * (flip * (compose * f * car)) * cdr) * p}}
ASSERT(compose * (flip * (compose * flip * car)) * cdr * detail::pair_sample0 * detail::pair_sample0 == duplicate * (compose * (flip * (compose * flip * car)) * cdr) * detail::pair_sample0);
ASSERT(compose * (flip * (compose * duplicate * car)) * cdr * detail::pair_sample1 * detail::pair_sample1 == duplicate * (compose * (flip * (compose * duplicate * car)) * cdr) * detail::pair_sample1);
//
//              = lambda(f) {duplicate * (compose * (flip * (compose * f * car)) * cdr)}
//              = lambda(f) {duplicate * (compose * (flip * (flip * compose * car * f)) * cdr)}
ASSERT(duplicate * (compose * (flip * (compose * flip * car)) * cdr) == duplicate * (compose * (flip * (flip * compose * car * flip)) * cdr));
ASSERT(duplicate * (compose * (flip * (compose * duplicate * car)) * cdr) == duplicate * (compose * (flip * (flip * compose * car * duplicate)) * cdr));
//              = lambda(f) {duplicate * (compose * (compose * flip * (flip * compose * car) * f) * cdr)}
ASSERT(duplicate * (compose * (flip * (flip * compose * car * flip)) * cdr) == duplicate * (compose * (compose * flip * (flip * compose * car) * flip) * cdr));
ASSERT(duplicate * (compose * (flip * (flip * compose * car * duplicate)) * cdr) == duplicate * (compose * (compose * flip * (flip * compose * car) * duplicate) * cdr));
//              = lambda(f) {duplicate * (compose * compose * (compose * flip * (flip * compose * car)) * f * cdr)}
ASSERT(duplicate * (compose * (compose * flip * (flip * compose * car) * flip) * cdr) == duplicate * (compose * compose * (compose * flip * (flip * compose * car)) * flip * cdr));
ASSERT(duplicate * (compose * (compose * flip * (flip * compose * car) * duplicate) * cdr) == duplicate * (compose * compose * (compose * flip * (flip * compose * car)) * duplicate * cdr));
//              = lambda(f) {duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr * f)}
ASSERT(duplicate * (compose * compose * (compose * flip * (flip * compose * car)) * flip * cdr) == duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr * flip));
ASSERT(duplicate * (compose * compose * (compose * flip * (flip * compose * car)) * duplicate * cdr) == duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr * duplicate));
//              = lambda(f) {compose * duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr) * f}
ASSERT(duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr * flip) == compose * duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr) * flip);
ASSERT(duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr * duplicate) == compose * duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr) * duplicate);

Value uncurry = compose * duplicate * (flip * (compose * compose * (compose * flip * (flip * compose * car))) * cdr);

ASSERT(uncurry * true_ * (cons * true_ * false_) == true_);
ASSERT(uncurry * false_ * (cons * true_ * false_) == false_);

//arg
Value code_arg(Value /*context*/, void* /*data*/, Value argument)
{
	ASSERT(0);
	exit(1);
	return argument;
}

Expression arg(char const* sym)
{
	return ExpressionNode::make(make_combinator(code_arg, symbol(sym).cell));
}

//substitute
//Sxyz = xz(yz)
Value substitute = compose * (compose * duplicate) * (compose * compose * flip);
ASSERT(substitute * compose * flip * duplicate == compose * duplicate * (flip * duplicate));
ASSERT(substitute * flip * compose * constant == flip * constant * (compose * constant));

//lambda
Expression lambda(char const* param, Expression expr)
{
	return ExpressionNode::make_lambda(arg(param), expr);
}

//ASSERT(identity == lambda("x", arg("x")));
ASSERT(lambda("x", compose) * fail == compose);
ASSERT(lambda("x", arg("x")) == identity);
ASSERT(lambda("x", compose * compose) * fail == compose * compose);
ASSERT(lambda("x", flip * arg("x")) * cons * true_ * false_ == cons * false_ * true_);
ASSERT(lambda("x", compose * arg("x") * (flip * arg("x"))) * fail == substitute * compose * flip * fail);
ASSERT(lambda("x", lambda("y", arg("x") * arg("y"))) * flip * compose == flip * compose);
ASSERT(lambda("x", lambda("y", arg("x") * arg("y") * (arg("y") * arg("x")))) * flip * compose == flip * compose * (compose * flip));

//fix
//(define applicative-order-y
//  (lambda (f)
//    ((lambda (x) 
//       (f (lambda (arg) ((x x) arg))))
//     (lambda (x) 
//       (f (lambda (arg) ((x x) arg)))))))
Value fix = lambda("f",
		lambda("x", arg("f") * lambda("y", (arg("x") * arg("x")) * arg("y"))) *
		lambda("x", arg("f") * lambda("y", (arg("x") * arg("x")) * arg("y"))));

namespace detail
{
	Value inc = cons * cons;

	Value recurse_sample_gen = lambda("t",
			lambda("f",
				lambda("l",
					((cons *
					  (constant * arg("l")) *
					  arg("f")) *
					 (identical * arg("t") * arg("l"))) *
					(inc * arg("l")))));

	Value initial = (cons * true_ * false_);
	Value recurse_sample0 = recurse_sample_gen * initial;

	Value second = (inc * initial);
	Value recurse_sample1 = recurse_sample_gen * second;

	Value third = (inc * second);

	Value fix_sample = fix * (recurse_sample_gen * third);
}

ASSERT(detail::recurse_sample0 * fail * detail::initial == detail::initial);
ASSERT(detail::recurse_sample1 * identity * detail::initial == detail::second);
ASSERT(detail::fix_sample * detail::initial == detail::third);

//tapply
Value tapply  = lambda("f", lambda("x",
			cons *
			((car * arg("f")) * (car * arg("x"))) *
			((cdr * arg("f")) * (cdr * arg("x")))));

//tcompose
Value tcompose = lambda("f", lambda("g", lambda ("x",
				tapply * arg("f") * (tapply * arg("g") * arg("x")))));

//tflip
Value tflip = lambda("f",
		cons *
		(flip * (car * arg("f"))) *
		(flip * (cdr * arg("f"))));

//tduplicate
Value tduplicate = lambda("f",
		cons *
		(duplicate * (car * arg("f"))) *
		(duplicate * (cdr * arg("f"))));

//tconstant
Value tconstant = constant;

Value sample_type = symbol("type!");
Value sample_value = symbol("value");
Value sample_instance = cons * sample_type * sample_value;

//check
Value check = lambda("m", lambda("c",
			(cons *
			 identity *
			 (fail * arg("m"))) *
			arg("c") * true_));

ASSERT(identity * (check * symbol("test_message") * true_) == true_);

//concept_tag
Value concept_tag = symbol("concept_requirement");

//concept_nil
Value concept_nil = cons * concept_tag * nil;

//conceptp
//Value conceptp = lambda("c", 

//concept_cons
//Value concept_cons = lambda("r", lambda("c",
//			check * symbol("not a concept") * (conceptp * arg("c"))

//concept_requirement
Value concept_requirement = lambda("name", lambda("type",
			cons * concept_tag * (cons * arg("name") * arg("type"))));

//model_nil
//model_cons
//model_provision
//model_lookup

//tlambda

//source
//trace

//module
//function

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
		return ExpressionNode::make(identity);
	if (subexpr.is_leaf())
		return ExpressionNode::make(ExpressionNode::make(constant), subexpr);
	if (subexpr.is_lambda())
		return eliminate_lambda(ExpressionNode::make_lambda(param, eliminate_lambda(subexpr)));
	//T[Lx.(E1 E2)] => (S T[Lx.E1] T[Lx.E2])
	Expression subfunction = subexpr.ptr->function;
	Expression subargument = subexpr.ptr->argument;
	return ExpressionNode::make(ExpressionNode::make(ExpressionNode::make(substitute),
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
	return apply(context, evaluate(context, expr.ptr->function), evaluate(context, expr.ptr->argument));
}
