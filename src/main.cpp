#include <stdio.h>
#include <stdlib.h>
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

    char* tag_models = intern("models");
    char* tag_symbol = intern("symbol");
    char* tag_predicate = intern("predicate");
    char* tag_binding = intern("binding");
    char* tag_nothing = intern("nothing");
    char* tag_sequence = intern("sequence");

    struct Expression {
      Cell* cell;

      explicit Expression(Cell* cell): cell(cell) {}

      Expression& operator=(Expression const& other) {
        this->cell = make_cell(tag_binding, make_cell(this->cell, make_cell(other.cell, 0)));
        return *this;
      }

      Expression operator()(Expression const& x0) {
        return Expression(make_cell(tag_predicate, make_cell(this->cell, make_cell(x0.cell, 0))));
      }

      Expression operator()(Expression const& x0, Expression const& x1) {
        return Expression(make_cell(tag_predicate, make_cell(this->cell, make_cell(x0.cell, make_cell(x1.cell, 0)))));
      }

      Expression operator^=(Expression const& r) {
        return Expression(make_cell(tag_models, make_cell(this->cell, r.cell)));
      }
    };

    Expression operator,(Expression const& l, Expression const& r) {
    }

    Expression symbol(char const* s) {
      return Expression(make_cell(tag_symbol, intern(s)));
    }

    struct Value {
      Cell* cell;

      explicit Value(Cell* cell): cell(cell) {}
      Value(Expression const& expression);
    };

    Value evaluate(Value const& environment, Value const& expression);

    Value Nothing(make_cell(tag_nothing, 0));
    Value builtin_environment = Nothing;

    Value::Value(Expression const& expression) {
      *this = evaluate(builtin_environment, Value(expression.cell));
    }

    char* tagof(Value x) {
      return (char*)x.cell->head;
    }

    Value evaluate(Value const& environment, Value const& expression) {
      if (tagof(expression) == tag_predicate) {
        ASSERT(0);
      }
      ASSERT(0);
    }
  }

  using detail::Expression;
  using detail::Value;
  using detail::symbol;

  jest::Expression _ = jest::symbol("_");
}

#define JEST_DEFINE(x) jest::Expression x = jest::symbol(#x)

namespace testmodule {
  JEST_DEFINE(Void);
  JEST_DEFINE(OutputStream);
  JEST_DEFINE(InputStream);
  JEST_DEFINE(Data);
  JEST_DEFINE(NonnegativeInteger);
  JEST_DEFINE(_length);
  JEST_DEFINE(_write);
  JEST_DEFINE(_read);
  JEST_DEFINE(output);
  JEST_DEFINE(input);
  JEST_DEFINE(data);
  JEST_DEFINE(stream);
  JEST_DEFINE(write);
  JEST_DEFINE(read);
  JEST_DEFINE(copy);

  namespace detail {
    using namespace jest;

    Value module = (

      Data = _length(data ^= _) ^= NonnegativeInteger,
      OutputStream = _write(stream ^= _, data ^= Data) ^= Void,
      InputStream = _read(stream ^= _) ^= Data,

      write(output ^= OutputStream, data ^= Data) =
        _write(output, data),

      read(input ^= InputStream) =
        _read(input),

      copy(output ^= OutputStream, input ^= InputStream) =
        write(output, read(input)));
  }

  jest::Value module = detail::module;
}

int main() {
  using namespace jest;

  return 0;
}
