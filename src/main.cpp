#include <string>
#include <boost/variant.hpp>

using namespace std;
using namespace boost;

struct Symbol {
  string symbol;
  Symbol(string const& symbol): symbol(symbol) {}
};

struct Finite {
  Symbol symbol;
  explicit Finite(Symbol const& symbol): symbol(symbol) {}
};

struct Predicate;
struct Empty {};
Empty empty;
struct SequenceNode;

typedef variant<recursive_wrapper<SequenceNode>, Empty> Sequence;
typedef variant<recursive_wrapper<Predicate>, Sequence, Finite> Type;

struct Predicate {
  Symbol operator_;
  Type argument;
};

struct SequenceNode {
  Type head;
  Sequence tail;
};

struct PredicatePattern;
struct SequenceNodePattern;
struct Parameter;

struct EmptyPattern {};
EmptyPattern emptyPattern;

typedef variant<recursive_wrapper<SequenceNodePattern>, EmptyPattern> SequencePattern;
typedef variant<recursive_wrapper<PredicatePattern>, recursive_wrapper<Parameter>, SequencePattern, Type> Pattern;

struct SequenceNodePattern {
  Pattern head;
  SequencePattern tail;
};

struct Parameter {
  Pattern type;
  Symbol name;
};

struct PredicatePattern {
  Symbol operator_;
  Pattern argument;
};

struct PredicateExpression;
struct SequenceNodeExpression;

struct EmptyExpression {};
EmptyExpression emptyExpression;

struct FiniteExpression {
  Symbol symbol;
};

typedef variant<recursive_wrapper<SequenceNodeExpression>, EmptyExpression> SequenceExpression;
typedef variant<recursive_wrapper<PredicateExpression>, recursive_wrapper<SequenceExpression>,  FiniteExpression> Expression;

struct PredicateExpression {
  Symbol operator_;
  Expression argument;
};

struct SequenceNodeExpression {
  Expression head;
  SequenceExpression tail;
};

struct Binding {
  Type type;
  Pattern pattern;
  Expression expression;
};

int main() {
  SequencePattern pattern = emptyPattern;
  Sequence type = empty;
}
