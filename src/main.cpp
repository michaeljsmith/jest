#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <set>
#include <cassert>

struct Object;

struct Value
{
	Object* x;

	operator Value() {return x;}

	Value operator()();
	template <typename X0> Value operator()(X0 const& x0);
	template <typename X0, typename X1> Value operator()(X0 const& x0, X1 const& x1);
	template <typename X0, typename X1, typename X2> Value operator()(X0 const& x0, X1 const& x1, X2 const& x2);
	template <typename X0, typename X1, typename X2, typename X3> Value operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3);
	template <typename X0, typename X1, typename X2, typename X3, typename X4> Value operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4);
	template <typename X0, typename X1, typename X2, typename X3, typename X4, typename X5> Value operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4, X5 const& x5);

	bool operator==(Value const& other) const {return x == other.x;}
	bool operator!=(Value const& other) const {return x != other.x;}

	template <typename Idx> Value operator[](Idx idx);

	Value(Object* x): x(x) {}
};

typedef Value (* BuiltinFn)(Value args);

struct Object
{
	Object(BuiltinFn fn): fn(fn) {}
	BuiltinFn fn;
};

Value cell_object_(Value args) {assert(0);}
struct Cell : public Object
{
	Cell(Value head, Value tail):
		Object(cell_object_), head(head), tail(tail) {}
	Value head;
	Value tail;
};

Value cons_(Value args)
{
	using namespace std;

	typedef map<std::pair<Value, Value>, Value> CellMap;
	static CellMap cell_map;
	CellMap::iterator pos = cell_map.find(make_pair(head, tail));
	if (pos == cell_map.end())
	{
		Value cell = new Cell(head, tail);
		pos = cell_map.insert(make_pair(make_pair(head, tail), cell)).first;
	}

	return (*pos).second;
}

Value consp_(Value args)
{
	return x.x->type == cell_type;
}

Value car_(Value x)
{
	assert(consp(x));
	return ((Cell*)x.x)->head;
}

Value cdr_(Value x)
{
	assert(consp(x));
	return ((Cell*)x.x)->tail;
}

Value symbol_object_(Value args) {assert(0);}
struct Symbol : public Object
{
	Symbol(char const* sym): Object(symbol_type)
	{
		this->sym = (char*)malloc(strlen(sym) + 1);
		strcpy(this->sym, sym);
	}

	char* sym;
};

Value symbol(char const* s)
{
	using namespace std;

	typedef map<string, Value> SymMap;
	static SymMap symbol_map;
	string str(s);
	SymMap::iterator pos = symbol_map.find(str);
	if (pos == symbol_map.end())
	{
		Value symbol = new Symbol(s);
		pos = symbol_map.insert(make_pair(str, symbol)).first;
	}

	return (*pos).second;
}

Value nil = symbol("nil");
Value _f = symbol("_f");
Value _t = symbol("_t");

Value gensym()
{
	static int next_id = 100;
	char str[1024];
	sprintf(str, "@gensym%d", next_id++);
	return symbol(str);
}

int main()
{
	return 0;
}

Value Value::operator()() {return list(x);}
template <typename X0> Value Value::operator()(X0 const& x0) {return list(x, value(x0));}
template <typename X0, typename X1> Value Value::operator()(X0 const& x0, X1 const& x1) {return list(x, value(x0), value(x1));}
template <typename X0, typename X1, typename X2> Value Value::operator()(X0 const& x0, X1 const& x1, X2 const& x2) {return list(x, value(x0), value(x1), value(x2));}
template <typename X0, typename X1, typename X2, typename X3> Value Value::operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3) {return list(x, value(x0), value(x1), value(x2), value(x3));}
template <typename X0, typename X1, typename X2, typename X3, typename X4> Value Value::operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4) {return list(x, value(x0), value(x1), value(x2), value(x3), value(x4));}
template <typename X0, typename X1, typename X2, typename X3, typename X4, typename X5> Value Value::operator()(X0 const& x0, X1 const& x1, X2 const& x2, X3 const& x3, X4 const& x4, X5 const& x5) {return list(x, value(x0), value(x1), value(x2), value(x3), value(x4), value(x5));}

Value value(Object* x) {return Value(x);}
Value value(char const* x) {return symbol(x);}
