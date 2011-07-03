#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <string>

struct AssertRaiser
{
	AssertRaiser(bool condition, char const* file, int line, char const* msg)
	{
		if (!condition)
		{
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
		struct Cell
		{
			Cell(void* head, void* tail): head(head), tail(tail) {}
			void* head;
			void* tail;
		};

		Cell* make_cell(void* head, void* tail)
		{
			using namespace std;

			typedef map<std::pair<void*, void*>, Cell*> CellMap;
			static CellMap cell_map;
			CellMap::iterator pos = cell_map.find(make_pair(head, tail));
			if (pos == cell_map.end())
			{
				Cell* cell = new Cell(head, tail);
				pos = cell_map.insert(make_pair(make_pair(head, tail), cell)).first;
			}

			return (*pos).second;
		}

		ASSERT(make_cell(0, make_cell(0, 0)) == make_cell(0, make_cell(0, 0)));

		char* intern(char const* s)
		{
			using namespace std;

			typedef set<string> SymMap;
			static SymMap symbol_map;
			string str(s);
			SymMap::iterator pos = symbol_map.find(str);
			if (pos == symbol_map.end())
			{
				pos = symbol_map.insert(str).first;
			}

			return (char*)(*pos).c_str();
		}

		char* tag_finite = intern("tag_finite");
		char* tag_application = intern("tag_finite");
		char* tag_quote = intern("tag_quote");
		char* tag_conjunction = intern("tag_finite");
		char* tag_disjunction = intern("tag_finite");
		char* tag_implication = intern("tag_finite");
		char* tag_universal = intern("tag_finite");
		char* tag_existential = intern("tag_finite");

		struct Type {
			Cell* cell;
			explicit Type(Cell* cell): cell(cell) {}

			Type operator()(Type x0);
			Type operator()(Type x0, Type x1);
			Type operator&(Type const& t);
			Type operator,(Type const& t);
			Type operator|(Type const& t);
			Type operator=(Type const& t);
			Type operator&=(Type const& t);
			Type operator|=(Type const& t);
		};

		inline bool is_tagged(char const* tag, Type const& x) {
			return x.cell->head == tag;
		}

		inline bool is_finite(Type const& x) {
			return is_tagged(tag_finite, x);
		}

		inline Type finite_symbol(Type x) {
			ASSERT(is_finite(x));

			return Type(static_cast<Cell*>(x.cell->tail));
		}

		inline bool is_application(Type const& x) {
			return is_tagged(tag_application, x);
		}

		inline Type application_function(Type x) {
			ASSERT(is_finite(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->head));
		}

		inline Type application_argument(Type x) {
			ASSERT(is_application(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->tail));
		}

		inline bool is_quote(Type const& x) {
			return is_tagged(tag_quote, x);
		}

		inline Type quote_expression(Type x) {
			ASSERT(is_quote(x));

			return Type(static_cast<Cell*>(x.cell->tail));
		}

		inline bool is_conjunction(Type const& x) {
			return is_tagged(tag_conjunction, x);
		}

		inline Type conjunction_left(Type x) {
			ASSERT(is_conjunction(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->head));
		}

		inline Type conjunction_right(Type x) {
			ASSERT(is_conjunction(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->tail));
		}

		inline bool is_disjunction(Type const& x) {
			return is_tagged(tag_disjunction, x);
		}

		inline Type disjunction_left(Type x) {
			ASSERT(is_disjunction(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->head));
		}

		inline Type disjunction_right(Type x) {
			ASSERT(is_disjunction(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->tail));
		}

		inline bool is_implication(Type const& x) {
			return is_tagged(tag_implication, x);
		}

		inline Type implication_antecedent(Type x) {
			ASSERT(is_implication(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->head));
		}

		inline Type implication_consequent(Type x) {
			ASSERT(is_implication(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->tail));
		}

		inline bool is_universal(Type const& x) {
			return is_tagged(tag_universal, x);
		}

		inline Type universal_variable(Type x) {
			ASSERT(is_universal(x));

			Type v(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->head));
			ASSERT(is_finite(v));
			return v;
		}

		inline Type universal_expression(Type x) {
			ASSERT(is_universal(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->tail));
		}

		inline bool is_existential(Type const& x) {
			return is_tagged(tag_existential, x);
		}

		inline Type existential_variable(Type x) {
			ASSERT(is_existential(x));

			Type v(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->head));
			ASSERT(is_finite(v));
			return v;
		}

		inline Type existential_expression(Type x) {
			ASSERT(is_existential(x));

			return Type(static_cast<Cell*>(static_cast<Cell*>(x.cell->tail)->tail));
		}

		Type finite(char const* str);
		Type quote(Type const& x);

		Type apply(Type const& f, Type const& x);

		inline Type Type::operator()(Type x0) {
			return apply(*this, x0);
		}

		inline Type Type::operator()(Type x0, Type x1) {
			return apply(*this, x0 & x1);
		}

		inline Type Type::operator&(Type const& t) {
			return Type(make_cell(tag_conjunction, make_cell(this->cell, t.cell)));
		}

		inline Type Type::operator,(Type const& t) {
			return (*this) & t;
		}

		inline Type Type::operator|(Type const& t) {
			return Type(make_cell(tag_disjunction, make_cell(this->cell, t.cell)));
		}

		inline Type Type::operator=(Type const& t) {
			return Type(make_cell(tag_implication, make_cell(this->cell, t.cell)));
		}

		inline Type Type::operator&=(Type const& t) {
			ASSERT(this->cell->head == tag_finite);
			return Type(make_cell(tag_universal, make_cell(this->cell, t.cell)));
		}

		inline Type Type::operator|=(Type const& t) {
			ASSERT(this->cell->head == tag_finite);
			return Type(make_cell(tag_existential, make_cell(this->cell, t.cell)));
		}

		inline Type finite(char const* str) {
			return Type(make_cell(tag_finite, intern(str)));
		}

		Type quote(Type const& x) {
			return Type(make_cell(tag_finite, x.cell));
		}

		inline Type apply(Type const& f, Type const& x) {
			return Type(make_cell(tag_application, make_cell(f.cell, x.cell)));
		}

		Type evaluate(Type expression, Type environment);

		Type evaluate_quote(Type expression, Type /*environment*/) {
			return quote_expression(expression);
		}

		Type evaluate_finite(Type /*expression*/, Type /*environment*/) {
			ASSERT(0);
			return Type(0);
		}

		Type evaluate_application(Type /*expression*/, Type /*environment*/) {
			ASSERT(0);
			return Type(0);
		}

		Type evaluate_conjunction(Type expression, Type environment) {
			return evaluate(conjunction_left(expression), environment) &
				evaluate(conjunction_right(expression), environment);
		}

		Type evaluate_disjunction(Type expression, Type environment) {
			return evaluate(disjunction_left(expression), environment) |
				evaluate(disjunction_right(expression), environment);
		}

		Type evaluate_implication(Type expression, Type environment) {
		}

		Type evaluate_universal(Type expression, Type environment) {
		}

		Type evaluate_existential(Type expression, Type environment) {
		}

		Type evaluate(Type expression, Type environment) {
			if (is_quote(expression))
				return evaluate_quote(expression, environment);
			if (is_finite(expression))
				return evaluate_finite(expression, environment);
			if (is_application(expression))
				return evaluate_application(expression, environment);
			if (is_conjunction(expression))
				return evaluate_conjunction(expression, environment);
			if (is_disjunction(expression))
				return evaluate_disjunction(expression, environment);
			if (is_implication(expression))
				return evaluate_implication(expression, environment);
			if (is_universal(expression))
				return evaluate_universal(expression, environment);
			if (is_existential(expression))
				return evaluate_existential(expression, environment);
			return Error;
		}
	}

	using detail::Type;
	using detail::finite;

#define JEST_DEFINE(x) jest::Type x = jest::finite(#x)

	JEST_DEFINE(Void);
	JEST_DEFINE(Error);
}

namespace testmodule {
	using namespace jest;

	JEST_DEFINE(Copy);
	JEST_DEFINE(OutputStream);
	JEST_DEFINE(InputStream);
	JEST_DEFINE(Data);
	JEST_DEFINE(Write);
	JEST_DEFINE(Read);
	JEST_DEFINE(NonnegativeInteger);
	JEST_DEFINE(_length);
	JEST_DEFINE(_write);
	JEST_DEFINE(_read);

	namespace detail {
		JEST_DEFINE(a);
		JEST_DEFINE(b);
		JEST_DEFINE(c);

		Type module = (

				Data = a |= (
					_length(a) = (
						b |= (b = NonnegativeInteger))),

				OutputStream = a |= (
					b &=
					_write(a, b = Data) = (
						c |= (c = Void))),

				InputStream = a |= (
					b &=
					_read(a, b) = (
						c |= (c = Data))),

				a &= b &=
				Write(a = OutputStream, b = Data) = (
					_write(a, b)),

				a &=
				Read(a = InputStream) = (
					_read(a)),

				a &= b &=
					Copy(a=OutputStream, b=InputStream) = (
							Write(a, Read(b))));
	}

	Type module = detail::module;
}

int main() {
	using namespace jest;

	Type m = testmodule::module;
	return 0;
}
