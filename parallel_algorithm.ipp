#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>

namespace parallel_algorithm {

template <typename T, typename F>
std::vector<T> Transform(const std::vector<T>& input, F func) {
  std::vector<T> result(input.size());

  // TODO

  return result;
}

template <typename T, typename F>
T Reduce(const std::vector<T>& input, const T& init, F func) {
  // TODO

  return init;  // you probably want to change this
}

};  // namespace parallel_algorithm
