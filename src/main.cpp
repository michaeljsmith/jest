#include <string>
#include <stdio.h>
#include <boost/variant.hpp>
#include <tr1/tuple>

using namespace std;
using namespace boost;
using namespace std::tr1;

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

struct Symbol : string {
  explicit Symbol(string const& symbol): string(symbol) {}
};

ASSERT(Symbol("a") == Symbol("a"));
ASSERT(Symbol("a") != Symbol("b"));

struct Finite : Symbol {
  explicit Finite(Symbol const& symbol): Symbol(symbol) {}
};

ASSERT(Finite(Symbol("a")) == Finite(Symbol("a")));
ASSERT(Finite(Symbol("a")) != Finite(Symbol("b")));

struct Empty : tuple<> {};
Empty empty;
ASSERT(empty == empty);

struct Predicate;
struct SequenceNode;

typedef variant<recursive_wrapper<SequenceNode>, Empty> Sequence;
typedef variant<recursive_wrapper<Predicate>, Sequence, Finite> Object;

struct Predicate : tuple<Symbol, Object> {
  Predicate(Symbol const& operator_, Object const& argument): tuple<Symbol, Object>(operator_, argument) {}
};

struct SequenceNode : tuple<Object, Sequence> {
  SequenceNode(Object const& head, Sequence const& tail): tuple<Object, Sequence>(head, tail) {}
};

struct PredicatePattern;
struct SequenceNodePattern;
struct Parameter;

struct EmptyPattern : tuple<> {};
EmptyPattern emptyPattern;

typedef variant<recursive_wrapper<SequenceNodePattern>, EmptyPattern> SequencePattern;
typedef variant<recursive_wrapper<PredicatePattern>, recursive_wrapper<Parameter>, SequencePattern, Object> Pattern;

struct SequenceNodePattern : tuple<Pattern, SequencePattern> {
  SequenceNodePattern(Pattern const& head, SequencePattern const& tail): tuple<Pattern, SequencePattern>(head, tail) {}
};

struct Parameter : tuple<Symbol> {
  Parameter(Symbol const& name): tuple<Symbol>(name) {}

  Symbol getName() const {
    return get<0>(*this);
  }
};

struct PredicatePattern : tuple<Symbol, Pattern> {
  PredicatePattern(Symbol const& operator_, Pattern const& argument):
    tuple<Symbol, Pattern>(operator_, argument) {}
};

struct PredicateExpression;
struct SequenceNodeExpression;

struct EmptyExpression : tuple<> {};
EmptyExpression emptyExpression;

struct FiniteExpression : Symbol  {
  explicit FiniteExpression(Symbol const& symbol): Symbol(symbol) {}
};

typedef variant<recursive_wrapper<SequenceNodeExpression>, EmptyExpression> SequenceExpression;
typedef variant<recursive_wrapper<PredicateExpression>, recursive_wrapper<SequenceExpression>,  FiniteExpression> Expression;

struct PredicateExpression : tuple<Symbol, Expression> {
  PredicateExpression(Symbol const& operator_, Expression argument):
    tuple<Symbol, Expression>(operator_, argument) {}
};

struct SequenceNodeExpression : tuple<Expression, SequenceExpression> {
  SequenceNodeExpression(Expression const& head, SequenceExpression const& tail): tuple<Expression, SequenceExpression>(head, tail) {}
};

struct Binding : tuple<Object, Pattern, Expression> {
  Binding(Object const& object, Pattern const& pattern, Expression const& expression):
    tuple<Object, Pattern, Expression>(object, pattern, expression) {}
};

struct MatchBinding : tuple<Symbol, Object> {
  MatchBinding(Symbol const& symbol, Object const& object): tuple<Symbol, Object>(symbol, object) {}
};

struct MatchFail : tuple<> {};
MatchFail matchFail;

struct EmptyMatchList : tuple<> {};
EmptyMatchList emptyMatchList;

struct MatchBindingListNode;

typedef variant<recursive_wrapper<MatchBindingListNode>, EmptyMatchList> MatchBindingList;
typedef variant<MatchBindingList, MatchFail> MatchResult;

struct MatchBindingListNode : tuple<MatchBinding, MatchBindingList> {
  MatchBindingListNode(MatchBinding const& binding, MatchBindingList const& tail):
    tuple<MatchBinding, MatchBindingList>(binding, tail) {}
};

MatchBindingList consBindingList(MatchBinding const& binding, MatchBindingList const& tail)
{
  return MatchBindingList(MatchBindingListNode(binding, tail));
}

struct Matcher : public static_visitor<> {
  MatchResult result;

  Matcher(): result(emptyMatchList) {}

  void fail() {
    result = matchFail;
  }

  struct Binder : public static_visitor<MatchResult> {
    MatchBinding binding;
    Binder(MatchBinding const& binding): binding(binding) {}
    MatchResult operator()(MatchFail const& old) const {return old;}
    MatchResult operator()(MatchBindingList const& old) const {return consBindingList(binding, old);}
  };
  void bind(Symbol const& symbol, Object const& object) {
    result = apply_visitor(Binder(MatchBinding(symbol, object)), result);
  }

  template <typename P, typename O> void operator()(P const& /*pattern*/, O const& /*object*/) {
    fail();
  }
 
  //typedef variant<recursive_wrapper<PredicatePattern>, recursive_wrapper<Parameter>, SequencePattern, Object> Pattern;
  template <typename O> void operator()(Object const& pattern, O const& object) {
    if (!(pattern == Object(object)))
      fail();
  }

  template <typename O> void operator()(Parameter const& parameter, O const& object) {
    bind(parameter.getName(), object);
  }
};

MatchResult match(Pattern const& pattern, Object const& object) {
  Matcher matcher;
  apply_visitor(matcher, pattern, object);
  return matcher.result;
}

ASSERT(MatchResult(matchFail) == match(Object(Finite(Symbol("a"))), Finite(Symbol("b"))));
ASSERT(!(MatchResult(matchFail) == match(Object(Finite(Symbol("a"))), Finite(Symbol("a")))));
ASSERT(MatchResult(emptyMatchList) == match(Object(Finite(Symbol("a"))), Finite(Symbol("a"))));
ASSERT(MatchResult(consBindingList(MatchBinding(Symbol("a"), Finite(Symbol("b"))), emptyMatchList)) ==
    match(Parameter(Symbol("a")), Finite(Symbol("b"))));

int main() {
  //SequencePattern pattern = emptyPattern;
  //Sequence object = empty;
}
