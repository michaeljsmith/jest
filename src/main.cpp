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

    char* tag_error = intern("error");
    char* tag_models = intern("models");
    char* tag_symbol = intern("symbol");
    char* tag_predicate = intern("predicate");
    char* tag_conjunction = intern("conjunction");
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

      Expression operator^(Expression const& r) {
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

      bool operator==(Value const& other) const {
        return cell == other.cell;
      }

      operator bool() const;
    };

    Value evaluate(Value const& environment, Value const& expression);
    std::string value_to_string_recurse(Value expression);

    Value nil(make_cell(tag_symbol, intern("nil")));
    Value fail(make_cell(tag_symbol, intern("fail")));

    bool is_nil(Value const& value) {
      return value == nil;
    }

    Value car(Value pair) {
      return Value(static_cast<Cell*>(pair.cell->head));
    }

    Value cdr(Value pair) {
      return Value(static_cast<Cell*>(pair.cell->tail));
    }

    int length_recurse(int accumulator, Value const& value) {
      if (is_nil(value)) {
        return accumulator;
      } else {
        return length_recurse(accumulator + 1, car(value));
      }
    }

    int length(Value const& value) {
      return length_recurse(0, value);
    }

    Value::operator bool() const {
      ASSERT(cell != fail.cell);
      return cell != nil.cell;
    }

    Value cons(Value x0, Value x1) {
      return Value(make_cell(x0.cell, x1.cell));
    }

    Value Nothing(make_cell(tag_nothing, 0));

    Value builtin_environment = Nothing;

    Value::Value(Expression const& expression) {
      *this = evaluate(builtin_environment, Value(expression.cell));
    }

    char* tagof(Value x) {
      return (char*)x.cell->head;
    }

    bool is_error(Value expression) {
      return tagof(expression) == tag_error;
    }

    Value make_error(char const* message) {
      return Value(make_cell(tag_error, make_cell(tag_symbol, intern(message))));
    }

    Value make_error_format(char const* message, ...) {
      va_list args;
      va_start(args, message);
      char buf[1024];
      vsprintf(buf, message, args);
      va_end(args);
      return make_error(buf);
    }

    char* error_message(Value error) {
      ASSERT(is_error(error));
      return static_cast<char*>(static_cast<Cell*>(error.cell->tail)->tail);
    }

    bool is_nothing(Value const& expression) {
      return tag_nothing == tagof(expression);
    }

    bool is_symbol(Value expression) {
      return tagof(expression) == tag_symbol;
    }

    char* symbol_text(Value symbol) {
      ASSERT(is_symbol(symbol));
      return static_cast<char*>(symbol.cell->tail);
    }

    bool is_pair(Value expression) {
      return tagof(expression) == tag_pair;
    }

    Value make_pair(Value x0, Value x1) {
      if (is_error(x0))
        return x0;
      if (is_error(x1))
        return x1;
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
      if (is_error(symbol))
        return symbol;
      if (is_error(expression))
        return expression;
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
      ASSERT(!is_symbol(pattern));
      if (is_error(pattern))
        return pattern;
      if (is_error(expression))
        return expression;
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
      if (is_error(model))
        return model;
      if (is_error(concept))
        return concept;
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
      if (is_error(operator_))
        return operator_;
      if (is_error(operand))
        return operand;
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

    bool is_conjunction(Value expression) {
      return tagof(expression) == tag_conjunction;
    }

    Value make_conjunction(Value x0, Value x1) {
      if (is_error(x0))
        return x0;
      if (is_error(x1))
        return x1;
      return Value(make_cell(tag_conjunction, make_cell(x0.cell, x1.cell)));
    }

    Value conjunction_first(Value const& conjunction) {
      ASSERT(is_conjunction(conjunction));
      return Value(static_cast<Cell*>(static_cast<Cell*>(conjunction.cell->tail)->head));
    }

    Value conjunction_second(Value const& conjunction) {
      ASSERT(is_conjunction(conjunction));
      return Value(static_cast<Cell*>(static_cast<Cell*>(conjunction.cell->tail)->tail));
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
        return "Nothing";
      } else if (tag_error == tagof(expression)) {
        return string("Error(") + error_message(expression) + ")";
      } else {
        ASSERT(0);
        return "";
      }
    }

    Value compute_bindings_recurse(Value const& /*pattern*/, Value const& /*expression*/) {
      ASSERT(0);
      return nil;
    }

    Value compute_candidate(Value const& rule, Value const& expression) {
      ASSERT(is_rule(rule));
      Value bindings = compute_bindings_recurse(rule_pattern(rule), expression);
      if (bindings == fail) {
        return nil;
      } else {
        return cons(bindings, expression);
      }
    }

    Value get_predicate_candidate_list_recurse(Value candidate_list, Value const& environment, Value const& expression) {
      if (is_conjunction(environment)) {
        candidate_list = get_predicate_candidate_list_recurse(candidate_list, conjunction_first(environment), expression);
        return get_predicate_candidate_list_recurse(candidate_list, conjunction_second(environment), expression);
      } else if (is_rule(environment)) {
        if (Value candidate = compute_candidate(environment, expression)) {
          candidate_list = cons(candidate, candidate_list);
        }
        return candidate_list;
      } else if (is_nothing(environment)) {
        return candidate_list;
      } else {
        ASSERT(0);
        return candidate_list;
      }
    }

    Value get_predicate_candidate_list(Value const& environment, Value const& expression) {
      return get_predicate_candidate_list_recurse(nil, environment, expression);
    }

    Value evaluate_predicate_function(Value const& environment, Value const& bindings, Value const& expression) {
      Value subenvironment = make_conjunction(bindings, environment);
      return evaluate(subenvironment, expression);
    }

    Value evaluate_predicate(Value const& environment, Value const& expression) {
      Value candidate_list = get_predicate_candidate_list(environment, expression);

      if (is_nil(candidate_list)) {
        return make_error_format("No matching function for predicate \"%s\".",
            value_to_string_recurse(expression).c_str());
      } else if (length(candidate_list) > 1) {
        return make_error_format("Ambiguous predicate \"%s\".",
            value_to_string_recurse(expression).c_str());
      } else {
        Value candidate = car(candidate_list);
        Value bindings = car(candidate);
        Value rule_expression = cdr(candidate);
        return evaluate_predicate_function(environment, bindings, rule_expression);
      }
    }

    Value get_symbol_binding_list_recurse(Value binding_list, Value const& environment, Value const& symbol) {
      if (is_conjunction(environment)) {
        binding_list = get_symbol_binding_list_recurse(binding_list, conjunction_first(environment), symbol);
        return get_symbol_binding_list_recurse(binding_list, conjunction_second(environment), symbol);
      } else if (is_binding(environment)) {
        if (binding_symbol(environment) == symbol)
          return cons(binding_expression(environment), binding_list);
        return binding_list;
      } else if (is_nothing(environment)) {
        return binding_list;
      } else {
        ASSERT(0);
        return binding_list;
      }
    }

    Value get_symbol_binding_list(Value const& environment, Value const& symbol) {
      return get_symbol_binding_list_recurse(nil, environment, symbol);
    }

    Value evaluate_symbol(Value const& environment, Value const& symbol) {
      Value binding_list = get_symbol_binding_list(environment, symbol);

      if (is_nil(binding_list)) {
        return make_error_format("Undefined symbol \"%s\".", symbol_text(symbol));
      } else if (length(binding_list) > 1) {
        return make_error_format("Multiply defined symbol \"%s\".", symbol_text(symbol));
      } else {
        return evaluate(environment, car(binding_list));
      }
    }

    Value evaluate_pattern(Value const& environment, Value const& expression) {
      if (tag_predicate == tagof(expression)) {
        return make_predicate(
            evaluate_pattern(environment, predicate_operator(expression)),
            evaluate_pattern(environment, predicate_operand(expression)));
      } else if (tag_conjunction == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else if (tag_pair == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else if (tag_models == tagof(expression)) {
        return make_models(
            evaluate_pattern(environment, models_model(expression)),
            evaluate_pattern(environment, models_concept(expression)));
      } else if (tag_symbol == tagof(expression)) {
        return evaluate_symbol(environment, expression);
      } else if (tag_binding == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else if (tag_rule == tagof(expression)) {
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

    Value evaluate(Value const& environment, Value const& expression) {
      if (tag_predicate == tagof(expression)) {
        return evaluate_predicate(environment, expression);
      } else if (tag_conjunction == tagof(expression)) {
        ASSERT(0);
        return Value(0);
      } else if (tag_pair == tagof(expression)) {
        Value result0 = evaluate(environment, pair_first(expression));
        if (is_error(result0))
          return result0;
        Value new_environment = make_conjunction(result0, environment);
        return make_pair(
            result0, evaluate(new_environment, pair_second(expression)));
      } else if (tag_models == tagof(expression)) {
        return make_models(
            evaluate_pattern(environment, models_model(expression)),
            evaluate(environment, models_concept(expression)));
      } else if (tag_symbol == tagof(expression)) {
        return evaluate_symbol(environment, expression);
      } else if (tag_binding == tagof(expression)) {
        return make_binding(
            binding_symbol(expression),
            evaluate(environment, binding_expression(expression)));
        ASSERT(0);
        return Value(0);
      } else if (tag_rule == tagof(expression)) {
        return make_rule(
            evaluate_pattern(environment, rule_pattern(expression)),
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
  jest::Expression op = jest::symbol("op");
  jest::Expression Nothing = jest::symbol("Nothing");

  using detail::value_to_string_recurse;
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

      Data = Void,

      OutputStream = _write(stream ^ _, data ^ Data) ^ Void,
      InputStream = _read(stream ^ _) ^ Data,

      _write(output ^ OutputStream, data ^ Data) ^ Void,
      write(output ^ OutputStream, data ^ Data) ^ Void =
        _write(output, data),

      _read(input ^ InputStream) ^ Data,
      read(input ^ InputStream) ^ Data =
        _read(input),

      copy(output ^ OutputStream, input ^ InputStream) ^ Void =
        write(output, read(input)));
  }

  jest::Value module = detail::module;
}

int main() {
  using namespace jest;

  printf("%s\n", value_to_string_recurse(testmodule::module).c_str());

  return 0;
}
