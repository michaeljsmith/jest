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
#define ASSERT(x) AssertRaiser ASSERT_OBJ_NAME(assertObj, __LINE__) \
  ((x), __FILE__, __LINE__, #x);

namespace evaluation {
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
      Expression operator=(Expression const& other);
      Expression operator()(Expression const& other);
      Expression operator()(Expression const& x0, Expression const& x1);

      Cell* cell;
    };

    struct Value {
      Value(): cell(0) {}
      Value(Expression const& expression);
      explicit Value(Cell* cell): cell(cell) {}
      Cell* cell;
    };

    bool operator==(Value const& l, Value const& r);
    bool operator!=(Value const& l, Value const& r);

    int tag_symbol_dummy;
    void* tag_symbol = &tag_symbol_dummy;
    Value symbol(char const* text) {
      return Value(make_cell(tag_symbol, intern(text)));
    }

    Value nothing;
    Value fail = symbol("fail");

    bool failp(Value const& value) {
      return value == fail;
    }

    Value error = symbol("error");

    bool symbolp(Value value) {
      return value.cell && value.cell->head == tag_symbol;
    }

    bool nothingp(Value value) {
      return value == nothing;
    }

    int tag_cons_dummy;
    void* tag_cons = &tag_cons_dummy;
    Value cons(Value const& x0, Value const& x1) {
      return Value(make_cell(tag_cons, make_cell(x0.cell, x1.cell)));
    }

    bool consp(Value value) {
      return value.cell && value.cell->head == tag_cons;
    }

    Value car(Value value) {
      ASSERT(consp(value));
      return Value(static_cast<Cell*>(
            static_cast<Cell*>(value.cell->tail)->head));
    }

    Value cdr(Value value) {
      ASSERT(consp(value));
      return Value(static_cast<Cell*>(
            static_cast<Cell*>(value.cell->tail)->tail));
    }

    Value cadr(Value value) {
      ASSERT(consp(value));
      return car(cdr(value));
    }

    Value cddr(Value value) {
      ASSERT(consp(value));
      return cdr(cdr(value));
    }

    Value caddr(Value value) {
      ASSERT(consp(value));
      return car(cddr(value));
    }

    Value cdddr(Value value) {
      ASSERT(consp(value));
      return cdr(cddr(value));
    }

    Value list() {
      return nothing;
    }

    Value list(Value const& x0) {
      return cons(x0, list());
    }

    Value list(Value const& x0, Value const& x1) {
      return cons(x0, list(x1));
    }

    Value list(Value const& x0, Value const& x1, Value const& x2) {
      return cons(x0, list(x1, x2));
    }

    bool listp(Value value) {
      return nothingp(value) || consp(value);
    }

    char const* text(Value const& symbol) {
      ASSERT(symbolp(symbol));
      return static_cast<char const*>(symbol.cell->tail);
    }
    
    Value builtin_environment = nothing;

    Value evaluate(Value const& environment, Value const& expression);

    Value::Value(Expression const& expression) {
      *this = evaluate(builtin_environment, Value(expression.cell));
    }

    bool operator==(Value const& l, Value const& r) {
      return l.cell == r.cell;
    }

    bool operator!=(Value const& l, Value const& r) {
      return l.cell != r.cell;
    }

    Expression operator,(Expression const& x0, Expression const& x1) {
      Expression expression;
      expression.cell = list(symbol("sequence"), x0, x1).cell;
      return expression;
    }

    struct Evaluator {
      virtual std::pair<Value, Value> evaluate(Value const& environment, Value const& args) = 0;
    };

    template <typename T> struct FunctionEvaluator {};
    template <> struct FunctionEvaluator<std::pair<Value, Value>(*)(Value const&)> : public Evaluator {
      std::pair<Value, Value> (* func)(Value const&);
      FunctionEvaluator(std::pair<Value, Value> (* func)(Value const&)): func(func) {}
      virtual std::pair<Value, Value> evaluate(Value const& environment, Value const& args) {
        return func(environment);
      }
    };

    template <typename F> Evaluator* function_evaluator(F func) {
      return new FunctionEvaluator<F>(func);
    }

    std::pair<Value, Value> evaluate_fail(Value const& environment) {
      return std::make_pair(environment, fail);
    }

    namespace evaluator {
      Evaluator* fail() {
        function_evaluator(evaluate_fail);
      }
    }

    std::pair<Value, Value> evaluate_error(Value const& environment) {
      return std::make_pair(environment, error);
    }

    namespace evaluator {
      Evaluator* error() {
        function_evaluator(evaluate_error);
      }
    }

    std::pair<Value, Value> evaluate_head(Value const& environment, Value const& ) {
      return std::make_pair(environment, error);
    }

    namespace evaluator {
      Evaluator* evaluator = error();
    }

    Value evaluate(Value const& environment, Value const& expression) {
      std::pair<Value, Value> result = evaluator::evaluator->evaluate(environment, list(expression));
      return result.second;
    }

    Expression operator~(Expression const& expression) {
      Expression new_expression;
      new_expression.cell = list(symbol("declare"), Value(expression.cell)).cell;
      return new_expression;
    }
  }

  using detail::Value;
  using detail::Expression;
  using detail::error;
  using detail::list;
  using detail::symbol;

  Expression token(char const* text) {
    Expression expression;
    expression.cell = detail::symbol(text).cell;
    return expression;
  }

  Value quote(Expression const& expression) {
    Value value;
    value.cell = expression.cell;
    return value;
  }
}

namespace values {
  using evaluation::error;
}

#define JEST_DEFINE(x) evaluation::Expression x = evaluation::token(#x)

JEST_DEFINE(foo);
JEST_DEFINE(Foo);
JEST_DEFINE(Bar);
JEST_DEFINE(bar);
JEST_DEFINE(Bat);
JEST_DEFINE(bat);
JEST_DEFINE(baz);
JEST_DEFINE(spam);

namespace evaluation {
  ASSERT(foo == values::error);
  ASSERT(foo == evaluation::token("foo"));
  ASSERT(quote(~foo) == list(symbol("declare"), symbol("foo")));
  ASSERT(~foo == list(symbol("type"), symbol("foo")));
}

int main() {
  return 0;
}
