#include <set>
#include <map>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

char const* intern(char const* s)
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

	return (*pos).c_str();
}

struct Value
{
	Value(Cell* cell): cell(cell) {}

	Cell* cell;
};

typedef Value (* Code)(Value context, void* data, Value argument);

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

template <typename T0, typename T1> struct Apply
{
	typedef T0 Function;
	typedef T1 Argument;

	Function function;
	Argument argument;
};

Value make_combinator(Code code, void* data)
{
	return Value(new Cell(box_function(code), 0));
}

//fail
Value code_fail1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	fprintf(stderr, "fail\n");
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
Value code_constant1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	assert(f0);
	return apply(context, apply(context, Value(f0), argument), Value(f0));
}

Value code_constant(Value context, void* /*data*/, Value argument)
{
	return make_combinator(code_constant1, argument.cell);
}
Value constant = make_combinator(code_constant, 0);

//duplicate
Value code_duplicate1(Value context, void* data, Value argument)
{
	Cell* f0 = (Cell*)data;
	assert(f0);
	return f0;
}

Value code_duplicate(Value context, void* /*data*/, Value argument)
{
	return make_combinator(code_duplicate1, argument.cell);
}
Value duplicate = make_combinator(code_constant, 0);

//operator*
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

