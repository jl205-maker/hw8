#include <map>
#include <thread>
#include <numeric>
#include <algorithm>
#include <cctype>
#include <latch>
#include <barrier>
#include <future>
#include <thread>
#include <chrono>

#include "./catch.hpp"
#include "./parallel_algorithm.hpp"

TEST_CASE("int_square", "[transform]") {
  std::vector<int> nums {3, 5, 10, 2, 50, 12, 4324, 230, 120, 4325, 1232, 05235};

  auto square = [](int n) { return n * n; };
  
  auto actual = parallel_algorithm::Transform(nums, square);

  std::vector<int> expected(nums.size());
  std::transform(nums.begin(), nums.end(), expected.begin(), square);

  REQUIRE(actual == expected);
}

TEST_CASE("string_to_upper", "[transform]") {
  std::vector<std::string> strs {"hello", "TOE", "the", "book", "about", "my", "idle", "PLOT", "on", "a", "Vauge", "anxiety"};

  auto str_toupper = [](const std::string& n) -> std::string {
    std::string result = n;
    // effectively does:
    // for (it = begin; it != end; ++it) {
    //   *it = ::toupper(*it);
    // }
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
  };
  

  std::vector<std::string> expected(strs.size());
  std::transform(strs.begin(), strs.end(), expected.begin(), str_toupper);

  auto actual = parallel_algorithm::Transform(strs, str_toupper);

  REQUIRE(actual == expected);
}

// need to do this instead of std::therad::id
// since std::thread::id can be reused after a thread is done
using unique_tid = std::size_t;

static std::atomic<unique_tid> next_id = 1;
static thread_local unique_tid my_id = 0;

static unique_tid get_this_tid() {
  if(my_id == 0) {
    my_id = next_id++;
  }
  return my_id;
}


TEST_CASE("uses_threads", "[transform]") {

  // sets to the thread id of self
  auto set_tid = []([[maybe_unused]] unique_tid discarded) { return get_this_tid(); };

  auto check_size_n = [&set_tid](size_t n) {
    std::vector<unique_tid> tids(n);
    tids = parallel_algorithm::Transform(tids, set_tid);

    size_t num_threads = std::thread::hardware_concurrency();
    num_threads = num_threads == 0 ? 4 : num_threads;
    num_threads = n < num_threads ? n : num_threads; // if there are only n elements and n < num_threads then we only need n threads
    std::map<unique_tid, size_t> counts;

    for (const auto& id : tids) {
      counts[id] += 1;
    }

    // the main thread should be doing part of the work
    REQUIRE(counts.contains(get_this_tid()));

    // uses the number of threads defined by hardware concurrency
    REQUIRE(counts.size() == num_threads);

    size_t lower_bound = n / num_threads;
    size_t upper_bound = lower_bound + (n % num_threads);

    for (const auto& p : counts) {
      REQUIRE(p.second >= lower_bound);
      REQUIRE(p.second <= upper_bound);
    }
  };

  check_size_n(64);
  check_size_n(1);
  check_size_n(12);
  check_size_n(3400);

  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = num_threads == 0 ? 4 : num_threads;
  check_size_n(num_threads);
  check_size_n(num_threads * 2);
  check_size_n(num_threads * 3);
  check_size_n(num_threads * 4);
  check_size_n(num_threads * 3 + 17);
}

// tests to see if the expected number of threads are run in parallel
// on an input vector of size n
//
// does this by providing a dummy input and uses a latch.
// Barriers are initialized with a number N and threads will call arrive_and_wait()
// when the thread calls arrive_and_wait() it waits until a total of N threads
// arrive at the barrier, at which point they are all unblocked and can run.
void check_transform_parallel(size_t n) {
  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = num_threads == 0 ? 4 : num_threads;
  num_threads = n < num_threads ? n : num_threads; // if there are only n elements and n < num_threads then we only need n threads

  std::vector<int> nums(n); // this nums vector doesn't do anything

  // once threads have reached barrier once, set this bool to true
  // to make sure that threads don't have to wait for it again.
  std::atomic<bool> arrived_once = false;
  auto mark_done = [&arrived_once]() { arrived_once = true; };
  std::barrier b(num_threads, mark_done);

  auto arrive = [&b, &arrived_once]([[maybe_unused]] int discarded) {
    if (!arrived_once) {
      b.arrive_and_wait();
    }
    return 0;
  };

  // don't care about the return value
  auto _ = parallel_algorithm::Transform(nums, arrive);
};

// helper lambda for the function that checks if threads are run in parallel
// If they aren't run in parallel then the program gets stuck unless we
// enforce some sort of timeout.
void check_transform_parallel_timeout(size_t n) {
  std::packaged_task<void(size_t)> task(check_transform_parallel);
  auto future = task.get_future();
  std::jthread runner(std::move(task), n);
  
  bool timedout = false;
  if (future.wait_for(std::chrono::seconds(2)) != std::future_status::timeout) {
    runner.join();
    future.get();
  } else {
    timedout = true;
    runner.detach();
  }
  REQUIRE_FALSE(timedout);
};

TEST_CASE("is_parallel", "[transform]") {
  check_transform_parallel_timeout(64);
  check_transform_parallel_timeout(1);
  check_transform_parallel_timeout(12);
  check_transform_parallel_timeout(3400);

  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = num_threads == 0 ? 4 : num_threads;
  check_transform_parallel_timeout(num_threads);
  check_transform_parallel_timeout(num_threads * 2);
  check_transform_parallel_timeout(num_threads * 3);
  check_transform_parallel_timeout(num_threads * 4);
  check_transform_parallel_timeout(num_threads * 3 + 17);
}

TEST_CASE("int_sum", "[reduce]") {
  std::vector<int> nums {3, 5, 10, 2, 50, 12, 4324, 230, 120, 4325, 1232, 05235};
  
  int expected = std::reduce(nums.begin(), nums.end(), 0, std::plus<int>());
  auto actual = parallel_algorithm::Reduce(nums, 0, std::plus<int>());


  REQUIRE(actual == expected);
}

TEST_CASE("string_concat", "[reduce]") {
  std::vector<std::string> strs {"hello", "TOE", "the", "book", "about", "my", "idle", "PLOT", "on", "a", "Vauge", "anxiety"};

  auto str_concat = [](const std::string& lhs, const std::string& rhs) -> std::string {
    std::string result = lhs + rhs;
    return result;
  };
  

  std::string expected = std::reduce(strs.begin(), strs.end(), std::string(""), str_concat);

  auto actual = parallel_algorithm::Reduce(strs, std::string(""), str_concat);

  REQUIRE(actual == expected);
}

TEST_CASE("uses_threads", "[reduce]") {

  // sets to the thread id of self
  auto set_tid = [](const std::set<unique_tid>& first, const std::set<unique_tid>& second) -> std::set<unique_tid> {
    std::set<unique_tid> result = first;
    result.insert(second.begin(), second.end());
    result.insert(get_this_tid());
    return result;
  };

  auto check_size_n = [&set_tid](size_t n) {
    std::vector<std::set<unique_tid>> tids(n);
    std::set<unique_tid> final_tids = parallel_algorithm::Reduce(tids, {}, set_tid);

    size_t num_threads = std::thread::hardware_concurrency();
    num_threads = num_threads == 0 ? 4 : num_threads;
    size_t num_threads_floor = n / 3 < num_threads ? n / 3 : num_threads;
    num_threads_floor = num_threads_floor == 0 ? 1 : num_threads_floor; // min value of 1
    size_t num_threads_ceil = n / 2 < num_threads ? n / 2 : num_threads;
    num_threads_ceil = num_threads_ceil == 0 ? 1 : num_threads_ceil; // min value of 1
    // if there are only n elements and n < num_threads then we only need n threads
    // divide by 3 or 2 to handle how work is split up in a reducution

    // the main thread should be doing part of the work
    REQUIRE(final_tids.contains(get_this_tid()));

    // uses the number of threads defined by hardware concurrency
    REQUIRE(((final_tids.size() >= num_threads_floor) && (final_tids.size() <= num_threads_ceil)));
  };

  check_size_n(64);
  check_size_n(1);
  check_size_n(12);
  check_size_n(3400);
  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = num_threads == 0 ? 4 : num_threads;
  check_size_n(num_threads);
  check_size_n(num_threads * 2);
  check_size_n(num_threads * 3);
  check_size_n(num_threads * 4);
  check_size_n(num_threads * 3 + 17);
}

// since we don't have an exact number, use our own condition variable thing
// - barrier(min) -> could have race condition if people use more than the min threads
// - barruer(max) -> could have threads stuck waiting for threads that don't exist to show up.
void check_reduce_parallel(size_t n) {
  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = num_threads == 0 ? 4 : num_threads;
  size_t num_threads_floor = n / 3 < num_threads ? n / 3 : num_threads;
  num_threads_floor = num_threads_floor == 0 ? 1 : num_threads_floor; // min value of 1
  size_t num_threads_ceil = n / 2 < num_threads ? n / 2 : num_threads;
  num_threads_ceil = num_threads_ceil == 0 ? 1 : num_threads_ceil; // min value of 1

  std::vector<int> nums(n); // this nums vector doesn't do anything

  // once threads have reached barrier once, set this bool to true
  // to make sure that threads don't have to wait for it again.
  
  std::mutex m{};
  std::condition_variable cv{};
  bool arrived_once = false;
  size_t curr_waiting = 0;

  auto arrive = [&m, &cv, &arrived_once, &curr_waiting, num_threads_floor]([[maybe_unused]] int discarded1, [[maybe_unused]] int discarded2) {
    std::unique_lock ul(m);
    if (arrived_once) {
      return 0;
    }
    curr_waiting += 1;
    if (curr_waiting >= num_threads_floor) {
      arrived_once = true;
      curr_waiting -= 1;
      cv.notify_all();
      return 0;
    }

    while (!arrived_once) {
      cv.wait(ul);
    }
    curr_waiting -= 1;
    
    return 0;
  };

  // don't care about the return value
  auto _ = parallel_algorithm::Reduce(nums, 0, arrive);
};

// helper lambda for the function that checks if threads are run in parallel
// If they aren't run in parallel then the program gets stuck unless we
// enforce some sort of timeout.
void check_reduce_parallel_timeout(size_t n) {
  std::packaged_task<void(size_t)> task(check_reduce_parallel);
  auto future = task.get_future();
  std::jthread runner(std::move(task), n);
  
  bool timedout = false;
  if (future.wait_for(std::chrono::seconds(2)) != std::future_status::timeout) {
    runner.join();
    future.get();
  } else {
    timedout = true;
    runner.detach();
  }
  REQUIRE_FALSE(timedout);
};

TEST_CASE("is_parallel", "[reduce]") {
  check_reduce_parallel_timeout(64);
  check_reduce_parallel_timeout(1);
  check_reduce_parallel_timeout(12);
  check_reduce_parallel_timeout(3400);

  size_t num_threads = std::thread::hardware_concurrency();
  num_threads = num_threads == 0 ? 4 : num_threads;
  check_reduce_parallel_timeout(num_threads);
  check_reduce_parallel_timeout(num_threads * 2);
  check_reduce_parallel_timeout(num_threads * 3);
  check_reduce_parallel_timeout(num_threads * 4);
  check_reduce_parallel_timeout(num_threads * 3 + 17);
}
