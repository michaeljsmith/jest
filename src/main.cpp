// {{{ Prelude
#include <string>
#include <stdio.h>
#include <boost/variant.hpp>
#include <boost/none.hpp>
#include <tr1/tuple>

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
// }}}

// {{{ utils::lists::List
namespace utils {namespace lists {
  template <typename T> class List {
    struct Node {
      Node(T head, Node const* tail): head(head), tail(tail) {}
      T const head;
      Node const* const tail;
    };

    Node const* const node;

    template <typename U> friend List<U> nil();
    template <typename U> friend bool nilp(List<U> l);
    template <typename U> friend List<U> cons(U head, List<U> tail);
    template <typename U> friend bool consp(List<U> l);
    template <typename U> friend U car(List<U> l);
    template <typename U> friend List<U> cdr(List<U> l);

    explicit List(Node const* node): node(node) {}
  };

  template <typename T> bool nilp(List<T> l) {
    return l.node == nullptr;
  }

  template <typename T> List<T> nil() {
    return List<T>(nullptr);
  }

  template <typename T> bool consp(List<T> l) {
    return l.node != nullptr;
  }

  template <typename T> List<T> cons(T head, List<T> tail) {
    typedef typename List<T>::Node Node;
    return List<T>(new Node(head, tail.node));
  }

  template <typename T> T car(List<T> l) {
    ASSERT(!nilp(l));
    return l.node->head;
  }

  template <typename T> List<T> cdr(List<T> l) {
    ASSERT(!nilp(l));
    return List<T>(l.node->tail);
  }
}}
// }}}

// {{{ utils::trees::Tree
namespace utils {namespace trees {
  namespace detail {
    enum class Type {
      LEAF,
      BRANCH
    };
  }

  template <typename T> class Tree {

    struct Node;

    struct Leaf {
      explicit Leaf(T val): val(val) {}
      T const val;
    };

    struct Branch {
      Branch(Node const* left, Node const* right)
        : left(left), right(right) {}
      Node const* const left;
      Node const* const right;
    };

    struct Node {
      Node(Leaf leaf): type(detail::Type::LEAF) {
        new (&this->leaf) Leaf(leaf);
      }

      Node(Branch branch): type(detail::Type::BRANCH) {
        new (&this->branch) Branch(branch);
      }

      detail::Type type;
      union {
        Leaf leaf;
        Branch branch;
      };
    };

    explicit Tree(Node const* node): node(node) {
      ASSERT(node != nullptr);
    }

    Node const* const node;

    template <typename U> friend Tree<U> leaf(U val);
    template <typename U> friend Tree<U> branch(Tree<U> left, Tree<U> right);
    template <typename U> friend bool leafp(Tree<U> t);
    template <typename U> friend U leaf_val(Tree<U> t);
    template <typename U> friend Tree<U> left(Tree<U> t);
    template <typename U> friend Tree<U> right(Tree<U> t);
  };

  template <typename T> Tree<T> leaf(T val) {
    typedef typename Tree<T>::Leaf Leaf;
    typedef typename Tree<T>::Node Node;
    return Tree<T>(new Node(Leaf(val)));
  }

  template <typename T> Tree<T> branch(Tree<T> left, Tree<T> right) {
    typedef typename Tree<T>::Branch Branch;
    typedef typename Tree<T>::Node Node;
    return Tree<T>(new Node(Branch(left.node, right.node)));
  }

  template <typename T> bool leafp(Tree<T> t) {
    return t.node->type == detail::Type::LEAF;
  }

  template <typename T> T leaf_val(Tree<T> t) {
    ASSERT(leafp(t));
    return t.node->leaf.val;
  }

  template <typename T> bool branchp(Tree<T> t) {
    return !leafp(t);
  }

  template <typename T> Tree<T> left(Tree<T> t) {
    ASSERT(branchp(t));
    return Tree<T>(t.node->branch.left);
  }

  template <typename T> Tree<T> right(Tree<T> t) {
    ASSERT(branchp(t));
    return Tree<T>(t.node->branch.right);
  }
}}
// }}}

// {{{ utils::strings::String
namespace utils {namespace strings {
  struct String : public std::string {
    String(std::string s): std::string(s) {}
  };
}}
// }}}

// {{{ model::types::Type
namespace model {namespace types {
  class Type {
    typedef utils::strings::String String;
    typedef utils::trees::Tree<String> StringTree;

    Type(StringTree tree): tree(tree) {}

    StringTree tree;

    friend Type name(String name);
    friend bool namep(Type tp);
    friend String name_string(Type name);
    friend Type dependent(Type type_fn, Type type_arg);
    friend Type dependent_fn(Type tp);
    friend Type dependent_arg(Type tp);
  };

  inline Type name(Type::String name) {
    using namespace utils::trees;
    return Type(leaf(name));
  }

  inline bool namep(Type tp) {
    return leafp(tp.tree);
  }

  inline Type::String name_string(Type name) {
    ASSERT(namep(name));
    return leaf_val(name.tree);
  }

  inline Type dependent(Type type_fn, Type type_arg) {
    using namespace utils::trees;
    return Type(branch(type_fn.tree, type_arg.tree));
  }

  inline bool dependentp(Type tp) {
    return !namep(tp);
  }

  inline Type dependent_fn(Type tp) {
    ASSERT(dependentp(tp));
    return left(tp.tree);
  }

  inline Type dependent_arg(Type tp) {
    ASSERT(dependentp(tp));
    return right(tp.tree);
  }
}}
// }}}

// {{{ model::parameters::Parameter
namespace model {namespace parameters {
  class Parameter {
    typedef utils::strings::String String;
    typedef model::types::Type Type;

    Parameter(String name, Type type): name(name), type(type) {}

    String const name;
    Type const type;

    friend Parameter parameter(String name, Type type);
    friend String parameter_name(Parameter parameter);
    friend Type parameter_type(Parameter parameter);
  };

  inline Parameter parameter(Parameter::String name, Parameter::Type type) {
    return Parameter(name, type);
  }

  inline Parameter::String parameter_name(Parameter parameter) {
    return parameter.name;
  }

  inline Parameter::Type parameter_type(Parameter parameter) {
    return parameter.type;
  }
}}
// }}}

// {{{ model::expressions::Expression
namespace model {namespace expressions {
  class Expression {
    typedef utils::strings::String String;
    typedef utils::trees::Tree<String> StringTree;

    Expression(StringTree tree): tree(tree) {}

    StringTree tree;

    friend Expression ref(String name);
    friend bool refp(Expression tp);
    friend String ref_ident(Expression name);
    friend Expression form(Expression type_fn, Expression type_arg);
    friend Expression form_fn(Expression tp);
    friend Expression form_arg(Expression tp);
  };

  inline Expression ref(Expression::String name) {
    using namespace utils::trees;
    return Expression(leaf(name));
  }

  inline bool refp(Expression tp) {
    return leafp(tp.tree);
  }

  inline Expression::String ref_ident(Expression name) {
    ASSERT(refp(name));
    return leaf_val(name.tree);
  }

  inline Expression form(Expression type_fn, Expression type_arg) {
    using namespace utils::trees;
    return Expression(branch(type_fn.tree, type_arg.tree));
  }

  inline bool formp(Expression tp) {
    return !refp(tp);
  }

  inline Expression form_fn(Expression tp) {
    ASSERT(formp(tp));
    return left(tp.tree);
  }

  inline Expression form_arg(Expression tp) {
    ASSERT(formp(tp));
    return right(tp.tree);
  }
}}
// }}}

// {{{ model::expressions::Value
namespace model {namespace expressions {
  class Value {
    typedef utils::strings::String String;
    typedef utils::trees::Tree<String> StringTree;

    Value(StringTree tree): tree(tree) {}

    StringTree tree;

    friend Value sym(String string);
    friend bool symp(Value tp);
    friend String sym_string(Value symbol);
    friend Value comp(Value val_fn, Value val_arg);
    friend Value comp_fn(Value tp);
    friend Value comp_arg(Value tp);
  };

  inline Value sym(Value::String string) {
    using namespace utils::trees;
    return Value(leaf(string));
  }

  inline bool symp(Value val) {
    return leafp(val.tree);
  }

  inline Value::String sym_string(Value name) {
    ASSERT(symp(name));
    return leaf_val(name.tree);
  }

  inline Value comp(Value type_fn, Value type_arg) {
    using namespace utils::trees;
    return Value(branch(type_fn.tree, type_arg.tree));
  }

  inline bool compp(Value val) {
    return !symp(val);
  }

  inline Value comp_fn(Value val) {
    ASSERT(compp(val));
    return left(val.tree);
  }

  inline Value comp_arg(Value val) {
    ASSERT(compp(val));
    return right(val.tree);
  }
}}
// }}}

// {{{ model::functions::Function
namespace model {namespace functions {
  class Function {
    template <typename T> using List = utils::lists::List<T>;
    typedef model::types::Type Type;
    typedef model::parameters::Parameter Parameter;
    typedef model::expressions::Expression Expression;
    typedef List<Parameter const> ParameterList;

    Function(Type type, ParameterList params, Expression expr):
      type(type), params(params), expr(expr) {}

    Type const type;
    ParameterList const params;
    Expression const expr;

    friend Function fun(Type type, ParameterList params, Expression expr);
    friend Type fun_type(Function fn);
    friend ParameterList fun_params(Function fn);
    friend Expression fun_expr(Function fn);
  };

  Function fun(Function::Type type,
      Function::ParameterList params, Function::Expression expr) {
    return Function(type, params, expr);
  }

  Function::Type fun_type(Function fn) {
    return fn.type;
  }

  Function::ParameterList fun_params(Function fn) {
    return fn.params;
  }

  Function::Expression fun_expr(Function fn) {
    return fn.expr;
  }
}}
// }}}

// {{{ Testing
void test_lists() {
  using namespace utils::lists;

  ASSERT(nilp(nil<int>()));
  ASSERT(consp(cons(2, nil<int>())));
  ASSERT(2 == car(cons(2, nil<int>())));
  ASSERT(3 == car(cdr(cons(2, cons(3, nil<int>())))));
}

void test_trees() {
  using namespace utils::trees;

  ASSERT(3 == leaf_val(leaf(3)));
  ASSERT(leafp(leaf(3)));
  ASSERT(branchp(branch(leaf(2), leaf(3))));
  ASSERT(2 == leaf_val(left(branch(leaf(2), leaf(3)))));
  ASSERT(3 == leaf_val(right(branch(leaf(2), leaf(3)))));
}
// }}}

// {{{ Main
int main() {
  test_lists();
  test_trees();

  model::functions::Function* fn;
  return 0;
}
// }}}
