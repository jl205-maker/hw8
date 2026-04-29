#ifndef MESSAGE_QUEUE_HPP_
#define MESSAGE_QUEUE_HPP_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

///////////////////////////////////////////////////////////////////////////////
// A MessageQueue is a class that represents a queue of values mean to be
// passed from one thread to another.
//
// The queue supports:
// - adding a value to the end of the queue
// - removing a value from the front of the queue
// - removing a value from the front of the queue and waiting
//   for a request to be added if there isn't one already.
// The queue is thread safe, with no potential for data races, or deadlocks
///////////////////////////////////////////////////////////////////////////////
template <typename T>
class MessageQueue {
 public:
  // Constructor for a MessageQueue.
  // Initializes the queue to be empty
  // and ready to handle concurrent operations
  MessageQueue();

  // Destructor for MessageQueue.
  // Cleans up any remaining elements in the queue
  // and any synronization methods used for maintaining
  // the state of the queue.
  ~MessageQueue() = default;

  // Adds a value to the end of the queue
  // This operation is thread safe.
  //
  // Arguments:
  // - val: the value to add to the end of the queue
  //
  // Returns:
  // - true if the operation is successful
  // - false if the queue is closed
  bool Add(const T& val);

  // Closes the queue.
  //
  // Any calls to add() that happens after calling close should fail
  // and return false.
  //
  // calls to remove() or wait_remove() should return nullopt
  // if there are no elements in the queue left.
  //
  // Threads blocked on wait_remove() will be waken up to either
  // process any values left in the queue or return nullopt
  void Close();

  // Removes a value from the front of the queue
  // This operation is thread safe.
  //
  // Returns:
  // - the value if a value was successfully removed
  // - nullopt if there were no values in the queue
  std::optional<T> Remove();

  // Removes a value from the front of the queue but if there is no
  // value in the queue, calling thread will block or spin until there is
  // a value is available. If the the queue is closed and the queue is
  // empty, then it returns nullopt instead.
  //
  // This operation is thread safe.
  //
  // Arguments: None
  //
  // Returns:
  // - The value removed from the front of the queue
  // - nullopt if the queue is closed and empty
  std::optional<T> WaitRemove();

  // Returns the size of the queue currently
  // This operation is thread safe.
  //
  // Arguments: None
  //
  // Returns:
  // The size of (i.e. number of elements in) the queue
  int Size();

  // Disabling copying and moving for simplicity
  // In reality the move operations would make sense
  MessageQueue(const MessageQueue& other) = delete;
  MessageQueue& operator=(const MessageQueue& other) = delete;
  MessageQueue(MessageQueue&& other) = delete;
  MessageQueue& operator=(MessageQueue&& other) = delete;

 private:
  // Fields
  // TODO: Add any fields you need
  std::deque<T> m_que;  // you probably want to keep this. MessageQueue
                        // is just a wrapper around a queue anyways
                        // so you should use this
};

#include "./MessageQueue.ipp"

#endif  // MESSAGE_QUEUE_HPP_
