#include <stdio.h>

namespace DETAIL {
  template <typename T> struct Finite {
  };

  template <typename T> void output_preprocessed(Finite<T> const&) {
    printf("%s", get_label((T*)0));
  }

  template <typename T> struct TypeDeclaration {
    T type;
  };

  template <typename T> void output_preprocessed(TypeDeclaration<T> const& declaration) {
    printf("struct ");
    output_preprocessed(declaration.type);
    printf(" {};\n");
  }

  template <typename T> TypeDeclaration<T> operator~(T const&) {
    return TypeDeclaration<T>();
  }

  template <typename T> void output(T const& val) {
    output_preprocessed(val);
  };
}
#define JEST_DEFINE(x) namespace TYPES {struct x {}; char const* get_label(x const*) {return #x;};} DETAIL::Finite<TYPES::x> x;

#include PREDECLARED_FILE

int main() {
  DETAIL::output(

#include COLLATED_FILE

  );
  return 0;
}
