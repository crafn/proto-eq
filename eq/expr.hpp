#ifndef EQ_EXPR_HPP
#define EQ_EXPR_HPP

#include "util.hpp"

namespace eq {

class BaseVar;
template <typename T>
class Var;

template <typename T>
struct Expr {
	T value;

public:
	Expr(T t)
		: value(t) { }

	T get() { return value; }

	Set<BaseVar*> getVars() const { return value.getVars(); }

	explicit operator bool() const { return value.eval(); }

	auto eval() const
	-> decltype(value.eval())
	{ return value.eval(); }

};

template <typename T>
struct Expr<Var<T>> {
	Expr(Var<T>& value)
		: value(&value)
	{ }

	Var<T>& get() { return *value; }

	Set<BaseVar*> getVars() const { return {value}; }

	T eval() const { return value->get(); }

private:
	Var<T>* value;
};

template <typename T>
struct Constant {
	Constant(T value)
		: value(value)
	{ }

	T get() { return value; }

	Set<BaseVar*> getVars() const { return {}; }

	T eval() const { return value; }

private:
	T value;
};

template <typename E1, typename E2, typename Op>
struct BiOp {
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

template <typename T>
struct IsVar<Var<T>> { static constexpr bool value= true; };

/// @todo Simplify

template <typename T>
struct ToExpr {
	using PlainT= RemoveRef<RemoveConst<T>>;
	static Expr<PlainT> eval(T t) { return Expr<PlainT>{std::forward<T>(t)}; }
};

template <typename T>
struct ToExpr<T&> {
	using PlainT= RemoveRef<RemoveConst<T>>;
	static Expr<PlainT> eval(T& t) { return Expr<PlainT>{t}; }
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
struct ToExpr<const int> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<int&> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

} // detail

template <typename T>
constexpr bool isExpr() { return detail::IsExpr<T>::value; }

template <typename T>
constexpr bool isVar() { return detail::IsVar<T>::value; }

template <typename T1_, typename T2_>
constexpr bool isExprOpQuality()
{
	using T1= RemoveRef<RemoveConst<T1_>>;
	using T2= RemoveRef<RemoveConst<T2_>>;
	return isExpr<T1>() || isVar<T1>() || isExpr<T2>() || isVar<T2>();
}

/// T to Expr conversion
template <typename T>
constexpr auto expr(T&& t)
-> decltype(detail::ToExpr<T>::eval(std::forward<T>(t)))
{ return detail::ToExpr<T>::eval(std::forward<T>(t)); }

// Operations

struct Add {
	static constexpr bool isRelational= false;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs + rhs)
	{ return	lhs + rhs; }
};

struct Sub {
	static constexpr bool isRelational= false;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs - rhs)
	{ return	lhs - rhs; }
};

struct Eq {
	static constexpr bool isRelational= true;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs == rhs)
	{ return	lhs == rhs; }
};

struct Neq {
	static constexpr bool isRelational= true;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs != rhs)
	{ return	lhs != rhs; }
};

struct Gr {
	static constexpr bool isRelational= true;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs > rhs)
	{ return	lhs > rhs; }
};

struct Ls {
	static constexpr bool isRelational= true;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs < rhs)
	{ return	lhs < rhs; }
};

struct Geq {
	static constexpr bool isRelational= true;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs >= rhs)
	{ return	lhs >= rhs; }
};

struct Leq {
	static constexpr bool isRelational= true;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs <= rhs)
	{ return	lhs <= rhs; }
};

struct And {
	static constexpr bool isRelational= false;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs && rhs)
	{ return	lhs && rhs; }
};

struct Or {
	static constexpr bool isRelational= false;

	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs || rhs)
	{ return	lhs || rhs; }
};

/// @todo Not sure if needs perfect forwarding

/// +
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator+(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Add>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Add>{expr(e1), expr(e2)}; }

/// -
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator-(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Sub>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Sub>{expr(e1), expr(e2)}; }

/// ==
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator==(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Eq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Eq>{expr(e1), expr(e2)}; }

/// !=
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator!=(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Neq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Neq>{expr(e1), expr(e2)}; }

/// >
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator>(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Gr>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Gr>{expr(e1), expr(e2)}; }

/// <
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator<(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Ls>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Ls>{expr(e1), expr(e2)}; }

/// >=
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator>=(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Geq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Geq>{expr(e1), expr(e2)}; }

/// <=
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator<=(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Leq>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Leq>{expr(e1), expr(e2)}; }

/// &&
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator&&(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), And>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), And>{expr(e1), expr(e2)}; }

/// ||
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator||(E1&& e1, E2&& e2)
->	Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Or>>
{ return BiOp<decltype(expr(e1)), decltype(expr(e2)), Or>{expr(e1), expr(e2)}; }


namespace detail {

template <typename T>
struct IsRelation {
	static constexpr bool value= false;
};

template <typename E1, typename E2, typename Op>
struct IsRelation<Expr<BiOp<E1, E2, Op>>> {
	static constexpr bool value= Op::isRelational;
};

template <typename E1, typename E2>
struct IsRelation<Expr<BiOp<E1, E2, And>>> {
	static constexpr bool value= IsRelation<E1>::value && IsRelation<E2>::value;
};

template <typename E1, typename E2>
struct IsRelation<Expr<BiOp<E1, E2, Or>>> {
	static constexpr bool value= IsRelation<E1>::value && IsRelation<E2>::value;
};

} // detail

template <typename T>
constexpr bool isRelation() { return detail::IsRelation<T>::value; }


} // eq

#endif // EQ_EXPR_HPP
