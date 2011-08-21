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
      if (errorp(expression)) {
        return expression;
      }
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

    Value tag_scope = symbol("scope");
    bool scopep(Value const& value) {
      bool result = consp(value) && car(value) == tag_scope;
      if (result) {
        ASSERT(nothingp(cddr(value)));
      }
      return result;
    }

    Value scope_expression(Value const& scope) {
      ASSERT(scopep(scope));
      return cadr(scope);
    }

    Value tag_binding = symbol("binding");
    Value binding(Value const& symbol, Value const& value) {
      if (errorp(symbol)) {
        return symbol;
      } else if (errorp(value)) {
        return value;
      }
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

    Value tag_rule = symbol("rule");
    Value rule(Value const& pattern, Value const& expression) {
      if (errorp(pattern)) {
        return pattern;
      } else if (errorp(expression)) {
        return expression;
      }
      return list(tag_rule, pattern, expression);
    }

    bool rulep(Value value) {
      bool result = consp(value) && car(value) == tag_rule;
      if (result) {
        ASSERT(nothingp(cdddr(value)));
      }
      return result;
    }

    Value rule_pattern(Value const& rule) {
      ASSERT(rulep(rule));
      return cadr(rule);
    }

    Value rule_expression(Value const& rule) {
      ASSERT(rulep(rule));
      return caddr(rule);
    }

    Value tag_predicate = symbol("predicate");

    bool predicatep(Value value) {
      bool result = consp(value) && car(value) == tag_predicate;
      if (result) {
        ASSERT(nothingp(cdddr(value)));
      }
      return result;
    }

    Value predicate(Value const& operator_, Value const& argument) {
      if (errorp(operator_)) {
        return operator_;
      } else if (errorp(argument)) {
        return argument;
      }
      return list(tag_predicate, operator_, argument);
    }

    Value predicate_operator(Value const& predicate) {
      ASSERT(predicatep(predicate));
      return cadr(predicate);
    }

    Value predicate_argument(Value const& predicate) {
      ASSERT(predicatep(predicate));
      return caddr(predicate);
    }

    Value tag_typing = symbol("typing");
    Value typing(Value const& pattern, Value const& type) {
      if (errorp(pattern)) {
        return pattern;
      } else if (errorp(type)) {
        return type;
      }
      return list(tag_typing, pattern, type);
    }

    bool typingp(Value value) {
      bool result = consp(value) && car(value) == tag_typing;
      if (result) {
        ASSERT(nothingp(cdddr(value)));
      }
      return result;
    }

    Value typing_pattern(Value const& typing) {
      ASSERT(typingp(typing));
      return cadr(typing);
    }

    Value typing_type(Value const& typing) {
      ASSERT(typingp(typing));
      return caddr(typing);
    }

    Value tag_sequence = symbol("sequence");
    Value sequence(Value const& x0, Value const& x1) {
      if (errorp(x0)) {
        return x0;
      } else if (errorp(x1)) {
        return x1;
      }
      return list(tag_sequence, x0, x1);
    }

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

    Value tag_conjunction = symbol("conjunction");
    Value conjunction(Value const& x0, Value const& x1) {
      if (errorp(x0)) {
        return x0;
      } else if (errorp(x1)) {
        return x1;
      } else if (nothingp(x0)) {
        return x1;
      } else if (nothingp(x1)) {
        return x0;
      } else {
        return list(tag_conjunction, x0, x1);
      }
    }

    bool conjunctionp(Value const& value) {
      bool result = consp(value) && car(value) == tag_conjunction;
      if (result) {
        ASSERT(nothingp(cdddr(value)));
      }
      return result;
    }

    Value conjunction_first(Value const& conjunction) {
      ASSERT(conjunctionp(conjunction));
      return cadr(conjunction);
    }

    Value conjunction_second(Value const& conjunction) {
      ASSERT(conjunctionp(conjunction));
      return caddr(conjunction);
    }

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

    Value lookup_symbol_recurse(Value environment, Value symbol) {
      if (bindingp(environment)) {
        Value target = binding_symbol(environment);
        if (typingp(target) || symbolp(target)) {

          Value pattern, type;
          if (typingp(target)) {
            pattern = typing_pattern(target);
            type = typing_type(target);
          } else if (symbolp(target)) {
            pattern = target;
            type = nothing;
          } else {
            ASSERT(0);
          }

          if (pattern == symbol) {
            Value value = binding_value(environment);
            if (!nothingp(type)) {
              value = typing(value, type);
            }
            return value;
          } else {
            return fail;
          }
        } else {
          ASSERT(0);
          return nothing;
        }
      } else if (rulep(environment)) {
        return fail;
      } else if (nothingp(environment)) {
        return fail;
      } else if (conjunctionp(environment)) {
        Value result0 = lookup_symbol_recurse(
            conjunction_first(environment), symbol);
        if (result0 == error) {
          return result0;
        }
        Value result1 = lookup_symbol_recurse(
            conjunction_second(environment), symbol);
        if (result1 == error) {
          return result1;
        }

        if (result0 == fail && result1 == fail) {
          return fail;
        } else if (result0 != fail && result1 != fail) {
          return error;
        } else if (result0 != fail) {
          return result0;
        } else {
          return result1;
        }
      } else {
        ASSERT(0);
        return fail;
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
      Value symbol = binding_symbol(expression);

      Value target, type;
      if (typingp(symbol)) {
        type = typing_type(symbol);
        target = typing_pattern(symbol);
      } else if (symbolp(symbol)) {
        type = nothing;
        target = symbol;
      } else {
        ASSERT(0);
      }

      // Bind the symbol to itself, to allow definitions to be effectively
      // quoted.
      Value self_binding = binding(target, quotation(target));
      Value new_environment = conjunction(self_binding, environment);

      Value value = evaluate(new_environment, binding_value(expression));
      if (errorp(value)) {
        return value;
      }
      ASSERT(!failp(value));

      if (!nothingp(type)) {
        value = typing(value, type);
      }

      return binding(symbol, quotation(value));
    }

    Value match_pattern_recurse(Value const& arguments,
        Value const& pattern, Value const& value);

    Value match_pattern_type_recurse(Value const& arguments,
        Value const& pattern_type, Value const& value_type) {
      ASSERT(symbolp(pattern_type));
      ASSERT(symbolp(value_type));

      if (pattern_type == value_type) {
        return arguments;
      } else {
        ASSERT(0);
        return fail;
      }
    }

    Value match_pattern_typing_recurse(
        Value arguments, Value const& pattern, Value const& value) {
      ASSERT(typingp(pattern));
      if (!typingp(value)) {
        ASSERT(0);
        return fail;
      }

      Value result0 = match_pattern_type_recurse(
          arguments, typing_type(pattern), typing_type(value));
      if (failp(result0) || errorp(result0)) {
        ASSERT(0);
        return result0;
      }

      arguments = result0;

      Value symbol = typing_pattern(pattern);
      ASSERT(symbolp(symbol));
      Value new_binding = cons(symbol, typing_pattern(value));
      return cons(new_binding, arguments);
    }

    Value match_pattern_recurse(Value const& arguments,
        Value const& pattern, Value const& value) {
      ASSERT(!rulep(pattern));
      ASSERT(!bindingp(pattern));
      if (typingp(pattern)) {
        return match_pattern_typing_recurse(arguments, pattern, value);
      } else if (predicatep(pattern)) {
        ASSERT(0);
        return nothing;
      } else if (sequencep(pattern)) {
        ASSERT(0);
        return nothing;
      } else {
        ASSERT(0);
        return nothing;
      }
    }

    Value match_pattern(Value const& pattern, Value const& predicate) {
      return match_pattern_recurse(nothing, pattern, predicate);
    }

    Value match_rule(Value const& rule, Value const& predicate) {
      ASSERT(rulep(rule));
      ASSERT(predicatep(predicate));
      Value pattern = rule_pattern(rule);
      ASSERT(typingp(pattern));
      Value predicate_pattern = typing_pattern(pattern);
      Value type = typing_type(pattern);
      ASSERT(predicatep(predicate_pattern));
      if (predicate_operator(predicate_pattern) != predicate_operator(predicate)) {
        ASSERT(0);
        return fail;
      }

      Value arguments = match_pattern(
          predicate_argument(predicate_pattern), predicate_argument(predicate));
      if (arguments == fail) {
        return fail;
      }
      return list(type, arguments, rule_expression(rule));
    }

    Value lookup_rule_recurse(
        Value const& environment, Value const& predicate) {
      ASSERT(consp(environment));
      ASSERT(predicatep(predicate));

      if (nothingp(environment)) {
        ASSERT(0);
        return fail;
      } else if (conjunctionp(environment)) {
        Value x0 = lookup_rule_recurse(
            conjunction_first(environment), predicate);
        if (errorp(x0)) {
          return x0;
        }
        Value x1 = lookup_rule_recurse(
            conjunction_second(environment), predicate);
        if (errorp(x1)) {
          return x1;
        }

        if (x0 == fail && x1 == fail) {
          return fail;
        } else if (x0 != fail && x1 != fail) {
          return error;
        } else if (x0 != fail) {
          return x0;
        } else {
          return x1;
        }
      } else if (rulep(environment)) {
        return match_rule(environment, predicate);
      } else if (bindingp(environment)) {
        return fail;
      } else {
        ASSERT(!sequencep(environment));
        ASSERT(0);
        return fail;
      }
    }

    Value lookup_rule(Value const& environment, Value const& predicate) {
      Value result = lookup_rule_recurse(environment, predicate);
      if (result == fail) {
        return error;
      }
      return result;
    }

    Value bind_arguments_recurse(
        Value const& environment, Value const& arguments) {
      if (nothingp(arguments)) {
        return environment;
      }

      ASSERT(consp(arguments));
      Value argument = car(arguments);
      Value symbol = car(argument);
      ASSERT(symbolp(symbol));
      Value value = cdr(argument);
      Value new_binding = binding(symbol, quotation(value));
      Value new_environment = conjunction(new_binding, environment);
      return bind_arguments_recurse(new_environment, cdr(arguments));
    }

    Value bind_arguments(
        Value const& environment, Value const& arguments) {
      return bind_arguments_recurse(environment, arguments);
    }

    Value evaluate_predicate(
        Value const& environment, Value const& value) {
      ASSERT(predicatep(value));
      ASSERT(symbolp(predicate_argument(value)));
      ASSERT(predicate_argument(value) == symbol("baz"));
      Value argument_evaluated_predicate = predicate(
          predicate_operator(value),
          evaluate(environment, predicate_argument(value)));
      if (errorp(argument_evaluated_predicate)) {
        return argument_evaluated_predicate;
      }
      ASSERT(predicatep(argument_evaluated_predicate));
      Value result = lookup_rule(environment, argument_evaluated_predicate);
      if (errorp(result)) {
        return result;
      }
      ASSERT(consp(result));
      Value type = car(result);
      Value arguments = cadr(result);
      Value expression = caddr(result);
      ASSERT(nothingp(cdddr(result)));

      Value new_environment = bind_arguments(environment, arguments);
      return evaluate(new_environment, expression);
    }

    Value sequence_conjunction(Value const& sequence) {
      if (sequencep(sequence)) {
        return conjunction(
            sequence_conjunction(sequence_first(sequence)),
            sequence_conjunction(sequence_second(sequence)));
      } else {
        return sequence;
      }
    }

    Value evaluate_sequence(Value const& environment, Value const& value) {
      ASSERT(sequencep(value));
      Value result0 = evaluate(environment, sequence_first(value));
      if (errorp(result0)) {
        return result0;
      }
      Value new_environment = conjunction(
          environment, sequence_conjunction(result0));
      Value result1 = evaluate(new_environment, sequence_second(value));
      return sequence(result0, result1);
    }

    Value sequence_last(Value const& sequence) {
      if (sequencep(sequence)) {
        return sequence_last(sequence_second(sequence));
      } else {
        return sequence;
      }
    }

    Value evaluate_scope(Value const& environment, Value const& scope) {
      Value expression = scope_expression(scope);
      Value sequence_result = evaluate(environment, expression);
      if (errorp(sequence_result)) {
        return sequence_result;
      }
      ASSERT((sequencep(expression) && sequencep(sequence_result)) || 
          (!sequencep(expression) && !sequencep(sequence_result)));
      Value result = sequence_last(sequence_result);
      ASSERT(!sequencep(result));
      return result;
    }

    Value evaluate_typing(Value const& environment, Value const& expression) {
      ASSERT(typingp(expression));

      Value type = typing_type(expression);
      Value subresult = evaluate(environment, typing_pattern(expression));
      Value untyped_result;
      if (typingp(subresult)) {
        if (typing_type(subresult) != type) {
          ASSERT(symbolp(typing_type(subresult)));
          ASSERT(symbolp(type));
          fprintf(stderr, "%s\n", text(typing_type(subresult)));
          fprintf(stderr, "%s\n", text(type));
          ASSERT(0);
          return error;
        }

        untyped_result = typing_pattern(subresult);
      } else {
        ASSERT(0);
        untyped_result = subresult;
      }

      return typing(untyped_result, type);
    }

    Value evaluate(Value const& environment, Value const& expression) {
      if (quotationp(expression)) {
        return quotation_value(expression);
      } else if (sequencep(expression)) {
        return evaluate_sequence(environment, expression);
      } else if (symbolp(expression)) {
        return lookup_symbol(environment, expression);
      } else if (bindingp(expression)) {
        return evaluate_binding(environment, expression);
      } else if (predicatep(expression)) {
        return evaluate_predicate(environment, expression);
      } else if (typingp(expression)) {
        return evaluate_typing(environment, expression);
      } else if (rulep(expression)) {
        return expression;
      } else if (scopep(expression)) {
        return evaluate_scope(environment, expression);
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

      Value target(this->cell);
      Value value(other.cell);
      Value target_pattern = target;
      if (typingp(target_pattern)) {
        target_pattern = typing_pattern(target_pattern);
      }

      Expression expression;
      if (symbolp(target_pattern)) {
        expression.cell = binding(target, value).cell;
      } else if (predicatep(target_pattern)) {
        expression.cell = rule(target, value).cell;
      } else {
        ASSERT(0);
        expression.cell = 0;
      }
      return expression;
    }

    Expression Expression::operator()(Expression const& other) {
      ASSERT(this->cell->head == tag_symbol);

      Expression predicate;
      predicate.cell = make_cell(tag_cons, make_cell(
            tag_predicate.cell, make_cell(tag_cons, make_cell(
                this->cell, make_cell(
                  tag_cons, make_cell(other.cell, 0))))));
      return predicate;
    }

    Expression operator^(Expression const& l, Expression const& r) {
      Value x0(l.cell);
      ASSERT(symbolp(x0) || predicatep(x0));

      Expression typing;
      typing.cell = make_cell(tag_cons, make_cell(
            tag_typing.cell, make_cell(tag_cons, make_cell(
                l.cell, make_cell(tag_cons, make_cell(r.cell, 0))))));
      return typing;
    }

    Expression operator~(Expression x) {
      Expression scope;
      scope.cell = make_cell(tag_cons, make_cell(
            tag_scope.cell, make_cell(tag_cons, make_cell(x.cell, 0))));
      return scope;
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
JEST_DEFINE(bar);
JEST_DEFINE(Bat);
JEST_DEFINE(bat);
JEST_DEFINE(baz);

ASSERT(foo == values::error);
ASSERT(sequence_first(evaluation::quote((Foo, Bar))) == evaluation::quote(Foo));
ASSERT(sequence_first(evaluation::quote((Foo, Bar))) != evaluation::quote(Bar));
ASSERT(sequence_second(evaluation::quote((Foo, Bar))) == evaluation::quote(Bar));
ASSERT((Foo = Foo, Foo) != values::error);
ASSERT((Bar = Bar, foo ^ Bar = foo, foo) != values::error);
ASSERT((Bar = Bar, foo ^ Bar = foo, foo ^ Bar) != values::error);
ASSERT(typingp(~(Bar = Bar, foo ^ Bar = foo, foo ^ Bar)));
ASSERT(typingp(~(Bar = Bar, foo ^ Bar = foo, foo)));
ASSERT(~(Bar = Bar, foo ^ Bar = foo, foo ^ Bar) == evaluation::quote(foo ^ Bar));
ASSERT(~(Bar = Bar, foo ^ Bar = foo, foo) == evaluation::quote(foo ^ Bar));
ASSERT(~(Bar = Bar, foo ^ Bar = foo, foo) != evaluation::quote(foo ^ Bat));
ASSERT(~(Foo = Foo, Foo) == evaluation::quote(Foo));
ASSERT(~(Foo = Foo, Bar = Foo, Bar) == evaluation::quote(Foo));
ASSERT(typingp(~(
      Bat = Bat,
      bat ^ Bat = bat,
      (foo(bar ^ Bar) ^ Bat) = bat,
      baz ^ Bar = baz,
      foo(baz))));
ASSERT(~(
      Bat = Bat,
      bat ^ Bat = bat,
      (foo(bar ^ Bar) ^ Bat) = bat,
      baz ^ Bar = baz,
      foo(baz)) ==
    evaluation::quote(bat ^ Bat));

int main() {
  return 0;
}

