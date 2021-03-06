#ifndef EQ_EXPR_HPP
#define EQ_EXPR_HPP

#include "basevar.hpp"
#include "util.hpp"
#include "varhandle.hpp"

namespace eq {
namespace detail {

template <typename D1, typename D2>
struct PickDomain;

template <typename D>
struct PickDomain<void, D> { using Type= D; };

template <typename D>
struct PickDomain<D, void> { using Type= D; };

template <typename D>
struct PickDomain<D, D> { using Type= D; };

template <typename D1, typename D2>
struct PickDomain {
	static_assert(
			!sizeof(D1),
			"Expr contains different types of domains");
};

} // detail

template <typename D1, typename D2>
using PickDomain= typename detail::PickDomain<D1, D2>::Type;

template <typename T>
struct Expr {
	T value;

public:
	using Domain= typename T::Domain;

	Expr(T t)
		: value(t) { }

	T get() { return value; }

	Set<BaseVar*> getVars() const { return value.getVars(); }

	explicit operator bool() const { return value.eval(); }

	auto eval() const
	-> decltype(value.eval())
	{ return value.eval(); }

};

template <typename T, VarType type>
struct Expr<Var<T, type>> {
	using Domain= typename Var<T, type>::Domain;

	Expr(Var<T, type>& value)
		: handle(value)
	{ }

	Var<T, type>& get() const { return static_cast<Var<T, type>&>(handle.get()); }
	Set<BaseVar*> getVars() const { return {&handle.get()}; }
	T eval() const { return get(); }

private:
	VarHandle handle;
};

template <typename T>
struct Constant {
	using Domain= void;

	Constant(T value)
		: value(value)
	{ }

	T get() { return value; }

	Set<BaseVar*> getVars() const { return {}; }

	T eval() const { return value; }

private:
	T value;
};

/// Unary operator used in expression trees
template <typename E, typename Op>
struct UOp {
	using Domain= typename E::Domain;

	E e;

	UOp(E e)
		: e(e) { }

	Set<BaseVar*> getVars() const
	{ return e.getVars(); }

	auto eval() const
	-> decltype(Op::eval(e.eval()))
	{ return Op::eval(e.eval()); }
};

/// Binary operator used in expression trees
template <typename E1, typename E2, typename Op>
struct BiOp {
	using Domain= PickDomain<typename E1::Domain, typename E2::Domain>;

	E1 lhs;
	E2 rhs;

	BiOp(E1 lhs, E2 rhs)
		: lhs(lhs), rhs(rhs) { }

	Set<BaseVar*> getVars() const
	{ return lhs.getVars() + rhs.getVars(); }

	auto eval() const
	-> decltype(Op::eval(lhs.eval(), rhs.eval()))
	{ return Op::eval(lhs.eval(), rhs.eval()); }
};

namespace detail {

template <typename T>
struct IsExpr { static constexpr bool value= false; };

template <typename T>
struct IsExpr<Expr<T>> { static constexpr bool value= true; };

template <typename T>
struct IsVar { static constexpr bool value= false; };

template <typename T, VarType type>
struct IsVar<Var<T, type>> { static constexpr bool value= true; };

/// @todo Simplify

template <typename T>
struct ToExpr {
	using PlainT= RemoveConst<RemoveRef<T>>;
	static Expr<PlainT> eval(T t) { return Expr<PlainT>{std::forward<T>(t)}; }
};

template <typename T>
struct ToExpr<T&> {
	using PlainT= RemoveConst<RemoveRef<T>>;
	static Expr<PlainT> eval(T& t) { return Expr<PlainT>{t}; }
};

template <typename T, VarType type>
struct ToExpr<const Var<T, type>&> {
	using Type= const Var<T, type>&;
	using PlainT= RemoveConst<RemoveRef<Type>>;
	static Expr<PlainT> eval(Type& t)
	{
		/// Consider:
		///	  auto area() const { return width*height; } // Expr of const Vars
		/// It should be possible to use this in the following contexts:
		///	  if (obj.area() > 5) { ... } // Works
		///   rel(x == obj.area()); // Fails because of const members in Vars
		/// -> const machinery of C++ is too limited for this use case so we need
		/// to bypass it with const_cast.
		/// This is probably not so bad because we aren't trying to represent
		/// conventional state, but a handle to mathematical entity
		/// @todo Prevent instantiating const Var because it'll make this UB
		return Expr<PlainT>{const_cast<PlainT&>(t)};
	}
};

template <typename T>
struct ToExpr<Expr<T>> {
	static Expr<T> eval(Expr<T> t) { return t; }
};

template <typename T>
struct ToExpr<Expr<T>&> {
	static Expr<T> eval(Expr<T>& t) { return t; }
};

template <>
struct ToExpr<int> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<const int&> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<int&> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<double> {
	static Expr<Constant<double>> eval(double t)
	{ return Expr<Constant<double>>{t}; }
};

template <>
struct ToExpr<const double&> {
	static Expr<Constant<double>> eval(double t)
	{ return Expr<Constant<double>>{t}; }
};

template <>
struct ToExpr<double&> {
	static Expr<Constant<double>> eval(double t)
	{ return Expr<Constant<double>>{t}; }
};

template <>
struct ToExpr<bool> {
	static Expr<Constant<bool>> eval(int t)
	{ return Expr<Constant<bool>>{t}; }
};

template <>
struct ToExpr<const bool&> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<bool&> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

} // detail

template <typename T>
constexpr bool isExpr() { return detail::IsExpr<T>::value; }

template <typename T>
constexpr bool isVar() { return detail::IsVar<T>::value; }

template <typename T_>
constexpr bool isExprUOpQuality()
{
	using T= RemoveConst<RemoveRef<T_>>;
	return isExpr<T>() || isVar<T>();
}

template <typename T1_, typename T2_>
constexpr bool isExprBiOpQuality()
{
	using T1= RemoveConst<RemoveRef<T1_>>;
	using T2= RemoveConst<RemoveRef<T2_>>;
	return isExpr<T1>() || isVar<T1>() || isExpr<T2>() || isVar<T2>();
}

/// T to Expr conversion
template <typename T>
constexpr auto expr(T&& t)
-> decltype(detail::ToExpr<T>::eval(std::forward<T>(t)))
{ return detail::ToExpr<T>::eval(std::forward<T>(t)); }

// Basic operations

struct Add {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs + rhs)
	{ return	lhs + rhs; }
};

struct Sub {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs - rhs)
	{ return	lhs - rhs; }
};

struct Mul {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs * rhs)
	{ return	lhs * rhs; }
};

struct Div {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs / rhs)
	{ return	lhs / rhs; }
};

struct Pos {
	template <typename T>
	static auto eval(T&& rhs)
	-> decltype(+rhs)
	{ return	+rhs; }
};

struct Neg {
	template <typename T>
	static auto eval(T&& rhs)
	-> decltype(-rhs)
	{ return	-rhs; }
};

struct Eq {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs == rhs)
	{ return	lhs == rhs; }
};

struct Neq {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs != rhs)
	{ return	lhs != rhs; }
};

struct Gr {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs > rhs)
	{ return	lhs > rhs; }
};

struct Ls {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs < rhs)
	{ return	lhs < rhs; }
};

struct Geq {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs >= rhs)
	{ return	lhs >= rhs; }
};

struct Leq {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs <= rhs)
	{ return	lhs <= rhs; }
};

struct And {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs && rhs)
	{ return	lhs && rhs; }
};

struct Or {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs || rhs)
	{ return	lhs || rhs; }
};

struct Not {
	template <typename T>
	static auto eval(T&& rhs)
	-> decltype(!rhs)
	{ return	!rhs; }
};

/// @todo Not sure if needs perfect forwarding

/// +
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator+(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Add>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Add>{expr(e1), expr(e2)}; }

/// -
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator-(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Sub>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Sub>{expr(e1), expr(e2)}; }

/// *
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator*(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Mul>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Mul>{expr(e1), expr(e2)}; }

/// /
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator/(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Div>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Div>{expr(e1), expr(e2)}; }

/// Unary +
template <typename E, typename=
	EnableIf<isExprUOpQuality<E>()>>
auto operator+(E&& e)
->	Expr<UOp<decltype(expr(e)), Pos>>
{ return UOp<decltype(expr(e)), Pos>{expr(e)}; }

/// Unary -
template <typename E, typename=
	EnableIf<isExprUOpQuality<E>()>>
auto operator-(E&& e)
->	Expr<UOp<decltype(expr(e)), Neg>>
{ return UOp<decltype(expr(e)), Neg>{expr(e)}; }

/// ==
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator==(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Eq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Eq>{expr(e1), expr(e2)}; }

/// !=
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator!=(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Neq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Neq>{expr(e1), expr(e2)}; }

/// >
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator>(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Gr>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Gr>{expr(e1), expr(e2)}; }

/// <
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator<(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Ls>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Ls>{expr(e1), expr(e2)}; }

/// >=
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator>=(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Geq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Geq>{expr(e1), expr(e2)}; }

/// <=
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator<=(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Leq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Leq>{expr(e1), expr(e2)}; }

/// &&
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator&&(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), And>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), And>{expr(e1), expr(e2)}; }

/// ||
template <typename E1, typename E2, typename=
	EnableIf<isExprBiOpQuality<E1, E2>()>>
auto operator||(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Or>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Or>{expr(e1), expr(e2)}; }

/// !
template <typename E, typename=
	EnableIf<isExprUOpQuality<E>()>>
auto operator!(E&& e)
->	Expr<UOp<decltype(expr(e)), Not>>
{ return UOp<decltype(expr(e)), Not>{expr(e)}; }



namespace detail {

template <typename T>
struct IsRelation {
	static constexpr bool value= false;
};

template <typename E, typename Op>
struct IsRelation<Expr<UOp<E, Op>>> {
	static constexpr bool value= isSame<Return<decltype(&UOp<E, Op>::eval)>, bool>();
};

template <typename E1, typename E2, typename Op>
struct IsRelation<Expr<BiOp<E1, E2, Op>>> {
	static constexpr bool value= isSame<Return<decltype(&BiOp<E1, E2, Op>::eval)>, bool>();
};

} // detail

template <typename T>
constexpr bool isRelation() { return detail::IsRelation<T>::value; }


} // eq

#endif // EQ_EXPR_HPP
