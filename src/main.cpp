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

namespace utils {namespace lists {
  namespace detail {

    template <typename T> struct ListNode {
      ListNode(T head, ListNode const* tail): head(head), tail(tail) {}
      T const head;
      ListNode const* const tail;
    };

    template <typename T> struct List {
      ListNode<T> const* const node;

      template <typename U> friend List<U> nil();
      template <typename U> friend bool nilp(List<U> l);
      template <typename U> friend List<U> cons(U head, List<U> tail);
      template <typename U> friend bool consp(List<U> l);
      template <typename U> friend U car(List<U> l);
      template <typename U> friend List<U> cdr(List<U> l);

    private:
      List(): node(nullptr) {}
      List(T head, List tail): node(new ListNode<T>(head, tail.node)) {}
    };

    template <typename T> bool nilp(List<T> l) {
      return l.node == nullptr;
    }

    template <typename T> List<T> nil() {
      return List<T>();
    }

    template <typename T> bool consp(List<T> l) {
      return l.node != nullptr;
    }

    template <typename T> List<T> cons(T head, List<T> tail) {
      return List<T>(head, tail);
    }

    template <typename T> T car(List<T> l) {
      ASSERT(!nilp(l));
      return l.node->head;;
    }

    template <typename T> List<T> cdr(List<T> l) {
      ASSERT(!nilp(l));
      return l.node->tail;;
    }
  }

  using detail::List;
  using detail::nil;
  using detail::nilp;
  using detail::cons;
  using detail::consp;
  using detail::car;
  using detail::cdr;
}}

namespace utils {namespace trees {
  namespace detail {
    enum class Type {
      LEAF,
      BRANCH
    };

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
        Node(Leaf leaf): type(Type::LEAF) {
          new (&this->leaf) Leaf(leaf);
        }

        Node(Branch branch): type(Type::BRANCH) {
          new (&this->branch) Branch(branch);
        }

        Type type;
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
      return t.node->type == Type::LEAF;
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
  }

  using detail::leaf;
  using detail::branch;
  using detail::leafp;
  using detail::leaf_val;
  using detail::branchp;
  using detail::left;
  using detail::right;
}}

namespace model {namespace symbols {
  struct Symbol : public std::string {
    Symbol(std::string s): std::string(s) {}
  };
}}

namespace model {namespace types {
  struct Type;
}}

namespace model {namespace parameters {
  struct Parameter;
}}

namespace model {namespace expressions {
  struct Expression;
}}

namespace model {namespace functions {

  namespace detail {
    using namespace utils::lists;
    using namespace types;
    using namespace parameters;
    using namespace expressions;

    struct Function {
      Function(Type const* type, List<Parameter> const* params, Expression const* expr):
        type(type), params(params), expr(expr) {
        ASSERT(type != nullptr);
        ASSERT(params != nullptr);
        ASSERT(expr != nullptr);
      }

      Type const* const type;
      List<Parameter> const* const params;
      Expression const* const expr;
    };

  }

  using detail::Function;
}}

void test_lists() {
  using namespace utils::lists;

  ASSERT(nilp(nil<int>()));
  ASSERT(consp(cons(2, nil<int>())));
}

void test_trees() {
  using namespace utils::trees;

  ASSERT(3 == leaf_val(leaf(3)));
  ASSERT(leafp(leaf(3)));
  ASSERT(branchp(branch(leaf(2), leaf(3))));
  ASSERT(2 == leaf_val(left(branch(leaf(2), leaf(3)))));
  ASSERT(3 == leaf_val(right(branch(leaf(2), leaf(3)))));
}

int main() {
  test_lists();
  test_trees();

  model::functions::Function* fn;
  return 0;
}

