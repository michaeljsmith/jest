#include <string>
#include <cassert>

// {{{ utils::lists::List
namespace utils {namespace lists {
  template <typename T> class List {
    struct Node {
      Node(T head, Node const* tail): head_(head), tail_(tail) {}
      T const head_;
      Node const* const tail_;
    };

    Node const* const node_;

    template <typename U> friend List<U> nil();
    template <typename U> friend bool nilp(List<U> l);
    template <typename U> friend List<U> cons(U head, List<U> tail);
    template <typename U> friend bool consp(List<U> l);
    template <typename U> friend U car(List<U> l);
    template <typename U> friend List<U> cdr(List<U> l);

    explicit List(Node const* node): node_(node) {}
  };

  template <typename T> bool nilp(List<T> l) {
    return l.node_ == nullptr;
  }

  template <typename T> List<T> nil() {
    return List<T>(nullptr);
  }

  template <typename T> bool consp(List<T> l) {
    return l.node_ != nullptr;
  }

  template <typename T> List<T> cons(T head, List<T> tail) {
    typedef typename List<T>::Node Node;
    return List<T>(new Node(head, tail.node_));
  }

  template <typename T> T car(List<T> l) {
    assert(!nilp(l));
    return l.node_->head_;
  }

  template <typename T> List<T> cdr(List<T> l) {
    assert(!nilp(l));
    return List<T>(l.node_->tail_);
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
      explicit Leaf(T val): val_(val) {}
      T const val_;
    };

    struct Branch {
      Branch(Node const* left, Node const* right)
        : left_(left), right_(right) {}
      Node const* const left_;
      Node const* const right_;
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

    explicit Tree(Node const* node): node_(node) {
      assert(node != nullptr);
    }

    Node const* const node_;

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
    return Tree<T>(new Node(Branch(left.node_, right.node_)));
  }

  template <typename T> bool leafp(Tree<T> t) {
    return t.node_->type == detail::Type::LEAF;
  }

  template <typename T> T leaf_val(Tree<T> t) {
    assert(leafp(t));
    return t.node_->leaf.val_;
  }

  template <typename T> bool branchp(Tree<T> t) {
    return !leafp(t);
  }

  template <typename T> Tree<T> left(Tree<T> t) {
    assert(branchp(t));
    return Tree<T>(t.node_->branch.left_);
  }

  template <typename T> Tree<T> right(Tree<T> t) {
    assert(branchp(t));
    return Tree<T>(t.node_->branch.right_);
  }
}}
// }}}

// {{{ utils::strings::String
namespace utils {namespace strings {
  struct String : public std::string {
    String(std::string s): std::string(s) {}
  };

  inline String str(std::string s) {
    return String(s);
  }
}}
// }}}

// {{{ model::types::Type
namespace model {namespace types {
  class Type {
    typedef utils::strings::String String;
    typedef utils::trees::Tree<String> StringTree;

    Type(StringTree tree): tree_(tree) {}

    StringTree tree_;

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
    return leafp(tp.tree_);
  }

  inline Type::String name_string(Type name) {
    assert(namep(name));
    return leaf_val(name.tree_);
  }

  inline Type dependent(Type type_fn, Type type_arg) {
    using namespace utils::trees;
    return Type(branch(type_fn.tree_, type_arg.tree_));
  }

  inline bool dependentp(Type tp) {
    return !namep(tp);
  }

  inline Type dependent_fn(Type tp) {
    assert(dependentp(tp));
    return left(tp.tree_);
  }

  inline Type dependent_arg(Type tp) {
    assert(dependentp(tp));
    return right(tp.tree_);
  }
}}
// }}}

// {{{ model::parameters::Parameter
namespace model {namespace parameters {
  class Parameter {
    typedef utils::strings::String String;
    typedef model::types::Type Type;

    Parameter(String name, Type type): name_(name), type_(type) {}

    String const name_;
    Type const type_;

    friend Parameter parameter(String name, Type type);
    friend String parameter_name(Parameter parameter);
    friend Type parameter_type(Parameter parameter);
  };

  inline Parameter parameter(Parameter::String name, Parameter::Type type) {
    return Parameter(name, type);
  }

  inline Parameter::String parameter_name(Parameter parameter) {
    return parameter.name_;
  }

  inline Parameter::Type parameter_type(Parameter parameter) {
    return parameter.type_;
  }
}}
// }}}

// {{{ model::expressions::Expression
namespace model {namespace expressions {
  class Expression {
    typedef utils::strings::String String;

  public:
    enum class SubType {
      FORM,
      REFERENCE,
      SYMBOL_LITERAL,
      INTEGER_LITERAL
    };

  private:
    struct Node;

    struct Form {
      Form(Node const* fn, Node const* arg)
        : fn_(fn), arg_(arg) {}
      Node const* const fn_;
      Node const* const arg_;
    };

    struct Reference {
      explicit Reference(String ident): ident_(ident) {}
      String const ident_;
    };

    struct SymbolLiteral {
      explicit SymbolLiteral(String val): val_(val) {}
      String const val_;
    };

    struct IntegerLiteral {
      explicit IntegerLiteral(int val): val_(val) {}
      int const val_;
    };

    struct Node {
      Node(Form form): subtype(SubType::FORM) {
        new (&this->form) Form(form);
      }

      Node(Reference reference): subtype(SubType::REFERENCE) {
        new (&this->reference) Reference(reference);
      }

      Node(SymbolLiteral symbolLiteral): subtype(SubType::SYMBOL_LITERAL) {
        new (&this->symbolLiteral) SymbolLiteral(symbolLiteral);
      }

      Node(IntegerLiteral integerLiteral): subtype(SubType::INTEGER_LITERAL) {
        new (&this->integerLiteral) IntegerLiteral(integerLiteral);
      }

      SubType subtype;
      union {
        Form form;
        Reference reference;
        SymbolLiteral symbolLiteral;
        IntegerLiteral integerLiteral;
      };
    };

    explicit Expression(Node const* node): node_(node) {
      assert(node != nullptr);
    }

    Node const* const node_;

    friend Expression::SubType expr_subtype(Expression expr);
    friend Expression form(Expression fn, Expression arg);
    friend Expression symbolLiteral(String val);
    friend Expression integerLiteral(int val);
    friend Expression reference(String ident);
    friend String symbolLiteral_val(Expression expr);
    friend int integerLiteral_val(Expression expr);
    friend String reference_ident(Expression expr);
    friend Expression form_fn(Expression expr);
    friend Expression form_arg(Expression expr);
  };

  inline Expression::SubType expr_subtype(Expression expr) {
    return expr.node_->subtype;
  }

  inline Expression form(Expression fn, Expression arg) {
    return Expression(new Expression::Node(Expression::Form(fn.node_, arg.node_)));
  }

  inline Expression symbolLiteral(Expression::String value) {
    return Expression(new Expression::Node(Expression::SymbolLiteral(value)));
  }

  inline Expression integerLiteral(int val) {
    return Expression(new Expression::Node(Expression::IntegerLiteral(val)));
  }

  inline Expression reference(Expression::String ident) {
    return Expression(new Expression::Node(Expression::Reference(ident)));
  }

  inline Expression::String symbolLiteral_val(Expression expr) {
    assert(expr_subtype(expr) == Expression::SubType::SYMBOL_LITERAL);
    return expr.node_->symbolLiteral.val_;
  }

  inline int integerLiteral_val(Expression expr) {
    assert(expr_subtype(expr) == Expression::SubType::INTEGER_LITERAL);
    return expr.node_->integerLiteral.val_;
  }

  inline Expression::String reference_ident(Expression expr) {
    assert(expr_subtype(expr) == Expression::SubType::REFERENCE);
    return expr.node_->reference.ident_;
  }

  inline Expression form_fn(Expression expr) {
    assert(expr_subtype(expr) == Expression::SubType::FORM);
    return Expression(expr.node_->form.fn_);
  }

  inline Expression form_arg(Expression expr) {
    assert(expr_subtype(expr) == Expression::SubType::FORM);
    return Expression(expr.node_->form.arg_);
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
      type_(type), params_(params), expr_(expr) {}

    Type const type_;
    ParameterList const params_;
    Expression const expr_;

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
    return fn.type_;
  }

  Function::ParameterList fun_params(Function fn) {
    return fn.params_;
  }

  Function::Expression fun_expr(Function fn) {
    return fn.expr_;
  }
}}
// }}}

// {{{ model::values::Value
namespace model {namespace values {
  class Value {
    typedef utils::strings::String String;

  public:
    enum class SubType {
      COMPOSITE,
      SYMBOL,
      INTEGER
    };

  private:
    struct Node;

    struct Composite {
      Composite(Node const* fn, Node const* arg)
        : fn_(fn), arg_(arg) {}
      Node const* const fn_;
      Node const* const arg_;
    };

    struct Symbol {
      explicit Symbol(String val): val_(val) {}
      String const val_;
    };

    struct Integer {
      explicit Integer(int val): val_(val) {}
      int const val_;
    };

    struct Node {
      Node(Composite composite): subtype(SubType::COMPOSITE) {
        new (&this->composite) Composite(composite);
      }

      Node(Symbol symbol): subtype(SubType::SYMBOL) {
        new (&this->symbol) Symbol(symbol);
      }

      Node(Integer integer): subtype(SubType::INTEGER) {
        new (&this->integer) Integer(integer);
      }

      SubType subtype;
      union {
        Composite composite;
        Symbol symbol;
        Integer integer;
      };
    };

    explicit Value(Node const* node): node_(node) {
      assert(node != nullptr);
    }

    Node const* const node_;

    friend Value::SubType val_subtype(Value val);
    friend Value composite(Value left, Value right);
    friend Value symbol(String val);
    friend Value integer(int val);
    friend String symbol_val(Value val);
    friend int integer_val(Value val);
    friend Value comp_fn(Value val);
    friend Value comp_arg(Value val);
  };

  inline Value::SubType val_subtype(Value val) {
    return val.node_->subtype;
  }

  inline Value composite(Value fn, Value arg) {
    return Value(new Value::Node(Value::Composite(fn.node_, arg.node_)));
  }

  inline Value symbol(Value::String val) {
    return Value(new Value::Node(Value::Symbol(val)));
  }

  inline Value integer(int val) {
    return Value(new Value::Node(Value::Integer(val)));
  }

  inline Value::String symbol_val(Value val) {
    assert(val_subtype(val) == Value::SubType::SYMBOL);
    return val.node_->symbol.val_;
  }

  inline int integer_val(Value val) {
    assert(val_subtype(val) == Value::SubType::INTEGER);
    return val.node_->integer.val_;
  }

  inline Value comp_fn(Value val) {
    assert(val_subtype(val) == Value::SubType::COMPOSITE);
    return Value(val.node_->composite.fn_);
  }

  inline Value comp_arg(Value val) {
    assert(val_subtype(val) == Value::SubType::COMPOSITE);
    return Value(val.node_->composite.arg_);
  }
}}
// }}}

// {{{ Testing
inline void test_lists() {
  using namespace utils::lists;

  assert(nilp(nil<int>()));
  assert(consp(cons(2, nil<int>())));
  assert(2 == car(cons(2, nil<int>())));
  assert(3 == car(cdr(cons(2, cons(3, nil<int>())))));
}

inline void test_trees() {
  using namespace utils::trees;

  assert(3 == leaf_val(leaf(3)));
  assert(leafp(leaf(3)));
  assert(branchp(branch(leaf(2), leaf(3))));
  assert(2 == leaf_val(left(branch(leaf(2), leaf(3)))));
  assert(3 == leaf_val(right(branch(leaf(2), leaf(3)))));
}

inline void test_values() {
  using namespace utils::strings;
  using namespace model::values;

  assert(val_subtype(integer(3)) == Value::SubType::INTEGER);
  assert(integer_val(integer(3)) == 3);
  assert(val_subtype(symbol(str("foo"))) == Value::SubType::SYMBOL);
  assert(symbol_val(symbol(str("foo"))) == "foo");
  assert(val_subtype(composite(integer(2), integer(3))) == Value::SubType::COMPOSITE);
  assert(integer_val(comp_fn(composite(integer(2), integer(3)))) == 2);
  assert(symbol_val(comp_arg(composite(integer(2), symbol(str("bar"))))) == "bar");
}
// }}}

// {{{ Main
int main() {
  test_lists();
  test_trees();
  test_values();

  return 0;
}
// }}}
