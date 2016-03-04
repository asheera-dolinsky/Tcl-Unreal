/*
*	 The MIT License (MIT)
*
*	 Copyright (c) 2015 Alisa Dolinsky
*
*	 Permission is hereby granted, free of charge, to any person obtaining a copy
*	 of this software and associated documentation files (the "Software"), to deal
*	 in the Software without restriction, including without limitation the rights
*	 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*	 copies of the Software, and to permit persons to whom the Software is
*	 furnished to do so, subject to the following conditions:
*
*	 The above copyright notice and this permission notice shall be included in all
*	 copies or substantial portions of the Software.
*
*	 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*	 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*	 SOFTWARE.
*/

#include <tuple>
using namespace std;

template <typename... T> using tuple_stripped_intermediary = tuple<typename remove_reference<T>::type...>;
template <typename... T> using tuple_stripped = tuple_stripped_intermediary<typename remove_cv<T>::type...>;

// Helpers
template<int...> struct index_tuple {};
template<int I, typename IndexTuple, typename ...Types> struct make_indexes_impl;
template<int I, int ...Indexes, typename T, typename ...Types>
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...> {
	typedef typename make_indexes_impl<I + 1, index_tuple<Indexes..., I>, Types...>::type type;
};
template<int I, int ...Indexes> struct make_indexes_impl<I, index_tuple<Indexes...> > { typedef index_tuple<Indexes...> type; };
template<typename ...Types> struct make_indexes : make_indexes_impl<0, index_tuple<>, Types...> {};
// Unpack and Apply
template<typename Ret, typename ...Args, int ...Indexes >
Ret apply_helper(Ret(*pf)(Args...), index_tuple<Indexes...>, tuple_stripped<Args...>&& tup) {
	return pf(forward<Args>(get<Indexes>(tup))...);
}
template<typename Ret, typename ...Args>
Ret apply(Ret(*pf)(Args...), const tuple_stripped<Args...>& tup) {
	return apply_helper(pf, typename make_indexes<Args...>::type(), tuple_stripped<Args...>(tup));
}
template<typename Ret, typename ...Args>
Ret apply(Ret(*pf)(Args...), tuple_stripped<Args...>&& tup) {
	return apply_helper(pf, typename make_indexes<Args...>::type(), forward<tuple_stripped<Args...>>(tup));
}