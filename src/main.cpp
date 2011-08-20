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

    bool errorp(Value const& value) {
      return value == error;
    }

    Value builtin_environment = nothing;

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

    Value tag_quote = symbol("quote");
    Value quotation(Value const& expression) {
      return list(tag_quote, expression);
    }

    bool quotationp(Value const& value) {
      bool result = consp(value) && car(value) == tag_quote;
      if (result) {
        ASSERT(nothingp(cddr(value)));
      }
      return result;
    }

    Value quotation_value(Value const& quotation) {
      ASSERT(quotationp(quotation));
      return cadr(quotation);
    }

    Value tag_binding = symbol("binding");
    Value binding(Value const& symbol, Value const& value) {
      return list(tag_binding, symbol, value);
    }

    bool bindingp(Value value) {
      bool result = consp(value) && car(value) == tag_binding;
      if (result) {
        ASSERT(nothingp(cdddr(value)));
      }
      return result;
    }

    Value binding_symbol(Value value) {
      ASSERT(bindingp(value));
      return cadr(value);
    }

    Value binding_value(Value value) {
      ASSERT(bindingp(value));
      return caddr(value);
    }

    Value tag_sequence = symbol("sequence");
    bool sequencep(Value const& value) {
      bool result = consp(value) && car(value) == tag_sequence;
      if (result) {
        ASSERT(nothingp(cdddr(value)));
      }
      return result;
    }

    Value sequence_first(Value const& sequence) {
      ASSERT(sequencep(sequence));
      return cadr(sequence);
    }

    Value sequence_second(Value const& sequence) {
      ASSERT(sequencep(sequence));
      return caddr(sequence);
    }

    Value conjunction(Value const& x0, Value const& x1) {
      if (nothingp(x0)) {
        return x1;
      } else if (nothingp(x1)) {
        return x0;
      } else {
        ASSERT(0);
        return nothing;
      }
    }

    Value evaluate(Value environment, Value expression);

    Value::Value(Expression const& expression) {
      *this = evaluate(builtin_environment, Value(expression.cell));
    }

    bool operator==(Value const& l, Value const& r) {
      return l.cell == r.cell;
    }

    bool operator!=(Value const& l, Value const& r) {
      return l.cell != r.cell;
    }

    Value lookup_symbol_recurse(Value environment, Value symbol) {
      if (bindingp(environment)) {
        if (binding_symbol(environment) == symbol) {
          return binding_value(environment);
        } else {
          ASSERT(0);
          return fail;
        }
      } else if (nothingp(environment)) {
        return fail;
      } else {
        ASSERT(0);
        return nothing;
      }
    }

    Value lookup_symbol(Value environment, Value symbol) {
      Value result = lookup_symbol_recurse(environment, symbol);
      if (result == fail) {
        return error;
      } else {
        return evaluate(environment, result);
      }
    }

    Value evaluate_binding(Value environment, Value expression) {
      ASSERT(symbolp(binding_symbol(expression)));

      Value symbol = binding_symbol(expression);

      // Bind the symbol to itself, to allow definitions to be effectively
      // quoted.
      Value self_binding = binding(symbol, quotation(symbol));
      Value new_environment = conjunction(self_binding, environment);

      Value value = evaluate(new_environment, binding_value(expression));
      if (errorp(value)) {
        return value;
      }
      ASSERT(!failp(value));
      return binding(symbol, quotation(value));
    }

    Value evaluate(Value environment, Value expression) {
      if (quotationp(expression)) {
        return quotation_value(expression);
      } else if (sequencep(expression)) {
        ASSERT(consp(expression));
        ASSERT(!bindingp(sequence_second(expression)));
        Value result0 = evaluate(environment, sequence_first(expression));
        if (errorp(result0)) {
          return result0;
        }
        Value new_environment = conjunction(environment, result0);
        ASSERT(symbolp(sequence_second(expression)));
        Value result = evaluate(new_environment, sequence_second(expression));
        ASSERT(symbolp(result));
        return result;
      } else if (symbolp(expression)) {
        return lookup_symbol(environment, expression);
      } else if (bindingp(expression)) {
        return evaluate_binding(environment, expression);
      } else {
        ASSERT(0);
        return nothing;
      }
    }

    Expression operator,(Expression const& x0, Expression const& x1) {
      Expression expression;
      expression.cell = make_cell(tag_cons, make_cell(
            tag_sequence.cell, make_cell(tag_cons, make_cell(
                x0.cell, make_cell(tag_cons, make_cell(x1.cell, 0))))));
      return expression;
    }

    Expression Expression::operator=(Expression const& other) {
      ASSERT(!bindingp(Value(this->cell)));
      ASSERT(!bindingp(Value(other.cell)));
      if (this->cell->head == tag_symbol) {
        Expression binding;
        binding.cell = make_cell(tag_cons, make_cell(
              tag_binding.cell, make_cell(tag_cons, make_cell(
                  this->cell, make_cell(
                    tag_cons, make_cell(other.cell, 0))))));
        return binding;
      } else {
        ASSERT(0);
        return *this;
      }
    }
  }

  using detail::Expression;
  using detail::error;

  Expression symbol(char const* text) {
    Expression expression;
    expression.cell = detail::make_cell(
        detail::tag_symbol, detail::intern(text));
    return expression;
  }

  detail::Value quote(Expression const& expression) {
    return detail::Value(expression.cell);
  }
}

namespace values {
  using evaluation::error;
}

#define JEST_DEFINE(x) evaluation::Expression x = evaluation::symbol(#x)

JEST_DEFINE(foo);
JEST_DEFINE(Foo);
JEST_DEFINE(Bar);

ASSERT(foo == values::error);
ASSERT(sequence_first(evaluation::quote((Foo, Bar))) == evaluation::quote(Foo));
ASSERT(sequence_first(evaluation::quote((Foo, Bar))) != evaluation::quote(Bar));
ASSERT(sequence_second(evaluation::quote((Foo, Bar))) == evaluation::quote(Bar));
ASSERT((Foo = Foo, Foo) != values::error);
ASSERT((Foo = Foo, Foo) == evaluation::quote(Foo));

int main() {
  return 0;
}

