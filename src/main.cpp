#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <map>
#include <set>
#include <string>

struct AssertRaiser {
  AssertRaiser(bool condition, char const* file, int line, char const* msg) {
    if (!condition) {
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

namespace jest {
  using std::map;
  using std::set;
  using std::string;

  namespace detail {
    struct Cell {
      Cell(void* head, void* tail): head(head), tail(tail) {}
      void* head;
      void* tail;
    };

    Cell* make_cell(void* head, void* tail) {
      using namespace std;

      typedef map<std::pair<void*, void*>, Cell*> CellMap;
      static CellMap cell_map;
      CellMap::iterator pos = cell_map.find(make_pair(head, tail));
      if (pos == cell_map.end()) {
        Cell* cell = new Cell(head, tail);
        pos = cell_map.insert(make_pair(make_pair(head, tail), cell)).first;
      }

      return (*pos).second;
    }

    ASSERT(make_cell(0, make_cell(0, 0)) == make_cell(0, make_cell(0, 0)));

    char* intern(char const* s) {
      using namespace std;

      typedef set<string> SymMap;
      static SymMap symbol_map;
      string str(s);
      SymMap::iterator pos = symbol_map.find(str);
      if (pos == symbol_map.end()) {
        pos = symbol_map.insert(str).first;
      }

      return (char*)(*pos).c_str();
    }

    struct Expression {
      Cell* cell;
    };

    struct Value {
      Value(): cell(0) {}
      explicit Value(Expression const& expression);
      Cell* cell;
    };

    Value nothing;

    Value builtin_environment = nothing;

    int tag_symbol_dummy;
    void* tag_symbol = &tag_symbol_dummy;
    Expression symbol(char const* text) {
      Expression expression;
      expression.cell = make_cell(tag_symbol, intern(text));
      return expression;
    }

    Value evaluate(Value environment, Value expression);

    Value::Value(Expression const& expression) {
      Value input;
      input.cell = expression.cell;
      *this = evaluate(builtin_environment, input);
    }

    Value evaluate(Value /*environment*/, Value /*expression*/) {
      return nothing;
    }

    bool operator==(Value const& l, Value const& r) {
      return l.cell == r.cell;
    }

    bool operator!=(Value const& l, Value const& r) {
      return l.cell != r.cell;
    }

    bool operator==(Expression const& l, Expression const& r) {
      return Value(l) == Value(r);
    }

    bool operator!=(Expression const& l, Expression const& r) {
      return Value(l) != Value(r);
    }
  }

  using detail::Expression;
  using detail::symbol;
}

#define JEST_DEFINE(x) jest::Expression x = jest::symbol(#x)

JEST_DEFINE(foo);
JEST_DEFINE(Foo);
JEST_DEFINE(error);

ASSERT(foo == error);
ASSERT((Foo = Foo, Foo) != error);

int main() {
  return 0;
}

