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
    char* tag_rule = intern("rule");
    char* tag_nothing = intern("nothing");
    char* tag_pair = intern("pair");

    struct Expression {
      Cell* cell;

      explicit Expression(Cell* cell): cell(cell) {}

      Expression& operator=(Expression const& other) {
        if (this->cell->head == tag_symbol)
          this->cell = make_cell(tag_binding, make_cell(this->cell, other.cell));
        else
          this->cell = make_cell(tag_rule, make_cell(this->cell, other.cell));
        return *this;
      }

      Expression operator()(Expression const& x0) {
        return Expression(make_cell(tag_predicate, make_cell(this->cell, x0.cell)));
      }

      Expression operator()(Expression const& x0, Expression const& x1) {
        return Expression(make_cell(tag_predicate, make_cell(this->cell, make_cell(tag_pair, make_cell(x0.cell, x1.cell)))));
      }

      Expression operator^=(Expression const& r) {
        return Expression(make_cell(tag_models, make_cell(this->cell, r.cell)));
      }
    };

    Expression operator,(Expression const& l, Expression const& r) {
      return Expression(make_cell(tag_pair, make_cell(l.cell, r.cell)));
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
    std::string value_to_string_recurse(Value expression);

    Value Nothing(make_cell(tag_nothing, 0));
    Value builtin_environment = Nothing;

    Value::Value(Expression const& expression) {
      printf("%s\n", value_to_string_recurse(Value(expression.cell)).c_str());
      *this = evaluate(builtin_environment, Value(expression.cell));
    }

    char* tagof(Value x) {
      return (char*)x.cell->head;
    }

    bool is_symbol(Value expression) {
      return tagof(expression) == tag_symbol;
    }

    bool is_pair(Value expression) {
      return tagof(expression) == tag_pair;
    }

    Value make_pair(Value x0, Value x1) {
      return Value(make_cell(tag_pair, make_cell(x0.cell, x1.cell)));
    }

    Value pair_first(Value const& pair) {
      ASSERT(is_pair(pair));
      return Value(static_cast<Cell*>(static_cast<Cell*>(pair.cell->tail)->head));
    }

    Value pair_second(Value const& pair) {
      ASSERT(is_pair(pair));
      return Value(static_cast<Cell*>(static_cast<Cell*>(pair.cell->tail)->tail));
    }

    bool is_binding(Value expression) {
      return tagof(expression) == tag_binding;
    }

    Value make_binding(Value symbol, Value expression) {
      ASSERT(is_symbol(symbol));
      return Value(make_cell(tag_binding, make_cell(symbol.cell, expression.cell)));
    }

    Value binding_symbol(Value const& binding) {
      ASSERT(is_binding(binding));
      return Value(static_cast<Cell*>(static_cast<Cell*>(binding.cell->tail)->head));
    }

    Value binding_expression(Value const& binding) {
      ASSERT(is_binding(binding));
      return Value(static_cast<Cell*>(static_cast<Cell*>(binding.cell->tail)->tail));
    }

    bool is_rule(Value expression) {
      return tagof(expression) == tag_rule;
    }

    Value make_rule(Value pattern, Value expression) {
      return Value(make_cell(tag_rule, make_cell(pattern.cell, expression.cell)));
    }

    Value rule_pattern(Value const& rule) {
      ASSERT(is_rule(rule));
      return Value(static_cast<Cell*>(static_cast<Cell*>(rule.cell->tail)->head));
    }

    Value rule_expression(Value const& rule) {
      ASSERT(is_rule(rule));
      return Value(static_cast<Cell*>(static_cast<Cell*>(rule.cell->tail)->tail));
    }

    bool is_models(Value expression) {
      return tagof(expression) == tag_models;
    }

    Value make_models(Value model, Value concept) {
      return Value(make_cell(tag_models, make_cell(model.cell, concept.cell)));
    }

    Value models_model(Value const& models) {
      ASSERT(is_models(models));
      return Value(static_cast<Cell*>(static_cast<Cell*>(models.cell->tail)->head));
    }

    Value models_concept(Value const& models) {
      ASSERT(is_models(models));
      return Value(static_cast<Cell*>(static_cast<Cell*>(models.cell->tail)->tail));
    }


    bool is_predicate(Value expression) {
      return tagof(expression) == tag_predicate;
    }

    Value make_predicate(Value operator_, Value operand) {
      return Value(make_cell(tag_predicate, make_cell(operator_.cell, operand.cell)));
    }

    Value predicate_operator(Value const& predicate) {
      ASSERT(is_predicate(predicate));
      return Value(static_cast<Cell*>(static_cast<Cell*>(predicate.cell->tail)->head));
    }

    Value predicate_operand(Value const& predicate) {
      ASSERT(is_predicate(predicate));
      return Value(static_cast<Cell*>(static_cast<Cell*>(predicate.cell->tail)->tail));
    }

    std::string value_to_string_recurse(Value expression) {
      using namespace std;

      if (tag_predicate == tagof(expression)) {
        return value_to_string_recurse(predicate_operator(expression)) +
          "(" + value_to_string_recurse(predicate_operand(expression)) + ")";
      } else if (tag_pair == tagof(expression)) {
        return value_to_string_recurse(pair_first(expression)) + ", " +
          value_to_string_recurse(pair_second(expression));
      } else if (tag_models == tagof(expression)) {
        return value_to_string_recurse(models_model(expression)) + " ^= " +
          value_to_string_recurse(models_concept(expression));
      } else if (tag_symbol == tagof(expression)) {
        return static_cast<char*>(expression.cell->tail);
      } else if (tag_binding == tagof(expression)) {
        return value_to_string_recurse(binding_symbol(expression)) + " = " +
          value_to_string_recurse(binding_expression(expression));
      } else if (tag_rule == tagof(expression)) {
        return value_to_string_recurse(rule_pattern(expression)) + " = " +
          value_to_string_recurse(rule_expression(expression));
      } else if (tag_nothing == tagof(expression)) {
        ASSERT(0);
        return "";
      } else {
        ASSERT(0);
        return "";
      }
    }

    Value evaluate_predicate(Value const& environment, Value const& expression) {
    }

    Value evaluate(Value const& environment, Value const& expression) {
      if (tag_predicate == tagof(expression)) {
        return evaluate_predicate(environment, expression);
      } else if (tag_pair == tagof(expression)) {
        return make_pair(
          evaluate(environment, pair_first(expression)),
          evaluate(environment, pair_second(expression)));
      } else if (tag_models == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else if (tag_symbol == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else if (tag_binding == tagof(expression)) {
        return make_binding(
            binding_symbol(expression),
            evaluate(environment, binding_expression(expression)));
        ASSERT(0);
        return Value(0);
      } else if (tag_rule == tagof(expression)) {
        return make_rule(
            evaluate(environment, rule_pattern(expression)),
            evaluate(environment, rule_expression(expression)));
        ASSERT(0);
        return Value(0);
      } else if (tag_nothing == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else {
        ASSERT(0);
        return Value(0);
      }
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
