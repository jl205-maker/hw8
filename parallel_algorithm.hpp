#ifndef PARALLEL_ALGORITHM_HPP_
#define PARALLEL_ALGORITHM_HPP_

#include <vector>

namespace parallel_algorithm {

// Performs the "map" higher order function in parallel.
// Uses up to std::thread::hardware_concurrency() threads to do the work
// or 4 threads if std::hardware_concurrency() evaluates to 0.
//
// Arguments:
// - input: the input vector that we are performing the map operation on
// - func: a function that takes in a type T and returns a T.
//         In other words it has the signature `T func(T thing)`
//         but T may be passed in by const ref. This function
//         will be invoked on each element of the input vector.
//
// Returns:
// a vector containg the results of the transform operation.
// If the input vector is:
//   {a, b, c, d, e, f}
// Then the returned value is:
//   {func(a), func(b), func(c), func(d), func(e), func(f)}
template <typename T, typename F>
std::vector<T> Transform(const std::vector<T>& input, F func);

// Performs the "reduce" higher order function in parallel.
// Uses up to std::thread::hardware_concurrency() threads to do the work
// or 4 threads if std::hardware_concurrency() evaluates to 0.
//
// Arguments:
// - input: the input vector that we are performing the reduce operation on
// - init: the initial value we use for
// - func: a function that takes in two inputs of type T and returns a T.
//         In other words it has the signature `T func(T thing1, T thing2)`
//         but T may be passed in by const ref.
//         This function will be invoked on elements of the vector and
//         intermediate results.
//         This function should be Associative but does not have to be
//         Commutative.
//
//         Associative:
//           func(func(x, y), z) == func(x, func(y, z))
//         Commutative:
//           func(x, y) == func(y, x)
//
// Returns:
// the results of the reduce operation.
// If the input vector is:
//     {a, b, c}
// and `initial` is:
//     z
// Then the returned value is equivalent to:
//   func(func(func(a, b), c), z)
// since func is assumed to be associative, then the functions
// may be called on the inputs differently to still get an equivalent result.
// e.g. it may actually do: func(a, func(b, func(c, z))) or something else
template <typename T, typename F>
T Reduce(const std::vector<T>& input, const T& init, F func);

};  // namespace parallel_algorithm

#include "./parallel_algorithm.ipp"

#endif  // PARALLEL_ALGORITHM_HPP_
