#include <set>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct FileAssertRaiser
{
	FileAssertRaiser(bool condition, char const* file, int line, char const* msg)
	{
		if (!condition)
		{
			fprintf(stderr, "%s(%d): %s\n", file, line, msg);
			exit(1);
		}
	}
};
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define ASSERT_OBJ_NAME_DETAIL(a, b) a##b
#define ASSERT_OBJ_NAME(a, b) ASSERT_OBJ_NAME_DETAIL(a, b)
#define ASSERT(x) namespace {FileAssertRaiser ASSERT_OBJ_NAME(assertObj, __LINE__)((x), __FILE__, __LINE__, #x);}

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

struct Value
{
	Value(Cell* cell): cell(cell) {}

	Cell* cell;
};

bool operator==(Value const& l, Value const& r)
{
	return l.cell == r.cell;
}

typedef Value (* Code)(Value context, void* data, Value argument);

template <typename T0, typename T1> struct Apply
{
	typedef T0 Function;
	typedef T1 Argument;

	Apply(T0 const& f, T1 const& a): function(f), argument(a) {}

	operator Value() const;

	Function function;
	Argument argument;
};

Value evaluate(Value context, Value v)
{
	return v;
}

template <typename T0, typename T1> Value evaluate(
		Value context, Apply<T0, T1> const& application)
{
	return apply(context, application.function, application.argument);
}

template <typename T0, typename T1> Apply<T0, T1>::operator Value() const
{
	return evaluate(Value(0), *this);
}

template <typename T0, typename T1>
Apply<T0, T1> make_apply(T0 const& x0, T1 const& x1)
{
	return Apply<T0, T1>(x0, x1);
}

Apply<Value, Value> operator*(Value const& x0, Value const& x1)
{
	return make_apply(x0, x1);
}

template <typename T00, typename T01, typename T10, typename T11>
Apply<Apply<T00, T01>, Apply<T10, T11> > operator*(
		Apply<T00, T01> const& x0, Apply<T10, T11> const& x1)
{
	return make_apply(x0, x1);
}

template <typename T0, typename T1>
Apply<Apply<T0, T1>, Value> operator*(Apply<T0, T1> const& x0, Value const& x1)
{
	return make_apply(x0, x1);
}

template <typename T0, typename T1>
Apply<Value, Apply<T0, T1> > operator*(Value const& x0, Apply<T0, T1> const& x1)
{
	return make_apply(x0, x1);
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
Value code_fail1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	fprintf(stderr, "fail\n");
	assert(0);
	exit(1);
	return argument;
}

Value code_fail(Value context, void* /*data*/, Value argument)
{
	return make_combinator(code_fail1, argument.cell);
}
Value fail = make_combinator(code_fail, 0);

//compose
Value code_compose2(Value context, void* data, Value argument)
{
	Cell* fs = (Cell*)data;
	assert(fs);
	Cell* f0 = (Cell*)(fs->head);
	assert(f0);
	Cell* f1 = (Cell*)(fs->tail);
	assert(f1);
	return apply(context, Value(f0), apply(context, Value(f1), argument));
}

Value code_compose1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	assert(f0);
	return make_combinator(code_compose2, make_cell(f0, argument.cell));
}

Value code_compose(Value context, void* /*data*/, Value argument)
{
	return make_combinator(code_compose1, argument.cell);
}

Value compose = make_combinator(code_compose, 0);

//flip
Value code_flip2(Value context, void* data, Value argument)
{
	Cell* fs = (Cell*)data;
	assert(fs);
	Cell* f0 = (Cell*)(fs->head);
	assert(f0);
	Cell* f1 = (Cell*)(fs->tail);
	assert(f1);
	return apply(context, apply(context, Value(f0), argument), Value(f1));
}

Value code_flip1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	assert(f0);
	return make_combinator(code_flip2, make_cell(f0, argument.cell));
}

Value code_flip(Value context, void* /*data*/, Value argument)
{
	return make_combinator(code_flip1, argument.cell);
}

Value flip = make_combinator(code_flip, 0);

//constant
Value code_constant1(Value context, void* data, Value /*argument*/)
{
	Cell* f0 = (Cell*)data;
	assert(f0);
	return f0;
}

Value code_constant(Value context, void* /*data*/, Value argument)
{
	assert(argument.cell);
	return make_combinator(code_constant1, argument.cell);
}

Value constant = make_combinator(code_constant, 0);

//duplicate
Value code_duplicate1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	assert(f0);
	return apply(context, apply(context, Value(f0), argument), argument);
}

Value code_duplicate(Value context, void* /*data*/, Value argument)
{
	return make_combinator(code_duplicate1, argument.cell);
}

Value duplicate = make_combinator(code_duplicate, 0);

ASSERT(compose * flip * constant * fail == flip * (constant * fail));
ASSERT(compose == flip * constant * fail * compose);
ASSERT(compose == constant * compose * fail);
ASSERT(duplicate * compose * fail == compose * fail * fail);

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

//cons
Value cons = compose * flip * (flip * identity);
ASSERT(identity * compose * fail == compose * fail);
ASSERT(flip * identity * compose * constant == constant * compose);
ASSERT(cons * fail * compose * true_ == fail);
ASSERT(cons * fail * compose * false_ == compose);

//car
Value car = flip * identity * true_;
ASSERT(car * (cons * fail * compose) == fail);

//cdr
Value cdr = flip * identity * false_;
ASSERT(cdr * (cons * fail * compose) == compose);

//symbol
Value code_symbol(Value context, void* /*data*/, Value argument)
{
	assert(0);
}

Value symbol(char const* str)
{
	return make_combinator(code_fail, intern(str));
}

//symbolp
Value code_symbolp(Value context, void* /*data*/, Value argument)
{
	assert(argument.cell);
	Code* box = (Code*)argument.cell->head;
	return boolean(*box == code_symbol);
}

//lambda

//fix

//source
//trace

//module
//function

int main()
{
	return 0;
}
