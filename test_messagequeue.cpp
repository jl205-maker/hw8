#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "./MessageQueue.hpp"
#include "./catch.hpp"

using std::cerr;
using std::endl;
using std::optional;
using std::ostream;
using std::string;

struct ThreadArg {
  MessageQueue<string>* queue;
  int& num_read;
  int& num_write;
  pthread_mutex_t& rw_lock;
};

static void read_strings(MessageQueue<string>& queue, int& num_read, int& num_write, std::mutex& rw_lock, bool& reader_result);

static void read_closed(MessageQueue<string>& queue, MessageQueue<string>& second, int& num_read, int& num_write, std::mutex& rw_lock, bool& reader_result);


constexpr const char* k_one{"one"};
constexpr const char* k_zero {"zero"};
constexpr const char* k_negative{"./negative"};
constexpr const char* k_pi{"./pi"};
constexpr const char* k_rs{"./RuneLite"};

TEST_CASE("Add_Remove", "[MessageQueue]") {
  MessageQueue<string>* q = new MessageQueue<string>();
  optional<string> opt{};

  // try removing before anything has happened
  opt = q->Remove();
  REQUIRE_FALSE(opt.has_value());

  REQUIRE(q->Add(k_one));
  opt = q->Remove();
  REQUIRE(opt.has_value());
  REQUIRE(opt.value() == k_one);
  

  REQUIRE(0 == q->Size());
  // try removing from an empty queue
  opt = q->Remove();
  REQUIRE_FALSE(opt.has_value());

  REQUIRE(q->Add(k_zero));
  REQUIRE(q->Add(k_rs));
  REQUIRE(q->Add(k_negative));
  REQUIRE(q->Add(k_pi));
  REQUIRE(4 == q->Size());

  opt = q->Remove();
  REQUIRE(opt.has_value());
  REQUIRE(k_zero == opt.value());
  REQUIRE(3 == q->Size());

  opt = q->Remove();
  REQUIRE(opt.has_value());
  REQUIRE(k_rs == opt.value());
  REQUIRE(2 == q->Size());

  // Delete q to make sure destructor works, then award points
  // if it doesn't crash
  delete q;

  // re-construct and test destructing a queue with 1 elment
  q = new MessageQueue<string>();
  REQUIRE(q->Add(k_pi));
  delete q;

  // re-construct and test destructing an empty queue
  q = new MessageQueue<string>();
  delete q;
}

TEST_CASE("WaitRemove", "[MessageQueue]") {
  // lock used for both num_write num_read and cerr
  std::mutex rw_lock;
  int num_read = -1;
  int num_write = 0;
  bool reader_result = true;

  MessageQueue<string> q;

  std::jthread reader(read_strings, std::ref(q), std::ref(num_read), std::ref(num_write), std::ref(rw_lock), std::ref(reader_result));

  // first test that the reader thread sleeps/waits
  // when it tries to read and there is nothing on the queue

  // sleep for a bit to give a chance for reader to start 
  // and to try and WaitRemove from q
  sleep(1);

  // ensure that the reader thread
  // has started and not read anything
  rw_lock.lock();
  while (num_read != 0) {
    rw_lock.unlock();
    sleep(1);
    rw_lock.lock();
  }
  rw_lock.unlock();

  // Add and sleep so that reader
  // has a chance to read
  REQUIRE(q.Add(k_pi));
  sleep(1);

  rw_lock.lock();
  num_write = 1;
  if (num_read != 1) {
    cerr << "WaitRemove seemingly doesn't notice the Addition of";
    cerr << "a string to the queue. Possible deadlock or not implemented";
    cerr << endl;
  }
  REQUIRE(1 == num_read);
  rw_lock.unlock();

  // next, test the case where there are already
  // values in the queue when the reader calls WaitRemove.
  rw_lock.lock();
  REQUIRE(q.Add(k_one));
  REQUIRE(q.Add(k_negative));
  REQUIRE(q.Add(k_zero));
  REQUIRE(q.Add(k_rs));
  num_write += 4;
  rw_lock.unlock();

  // sleep for a bit to give a chance for reader to start 
  // and to try and WaitRemove from q
  sleep(3);

  int secs_waited;
  for (secs_waited = 3; secs_waited < 30; secs_waited += 3) {
    rw_lock.lock();
    if (num_read == 5) {
      rw_lock.unlock();
      break;
    }
    rw_lock.unlock();
    sleep(3);
  }

  // make sure that we didn't time out
  // waiting for the reader thread to
  // read 4 more times.
  REQUIRE(secs_waited < 30);
  
  reader.join();

  // make sure the reader had no errors
  REQUIRE(reader_result);
}


TEST_CASE("Close", "[MessageQueue]") {
  // lock used for both num_write num_read and cerr
  // lock used for both num_write num_read and cerr
  std::mutex rw_lock;
  int num_read = 0;
  int num_write = 0;

  MessageQueue<string> q;
  MessageQueue<string> second;
  bool reader_result = true;

  std::jthread reader(read_closed, std::ref(q), std::ref(second), std::ref(num_read), std::ref(num_write), std::ref(rw_lock), std::ref(reader_result));

  // first test that the reader thread sleeps/waits
  // when it tries to read and there is nothing on the queue

  // sleep for a bit to give a chance for reader to start 
  // and to try and WaitRemove from q
  sleep(1);

  // ensure that the reader thread
  // has not read anything
  rw_lock.lock();
  REQUIRE(num_read == 0);
  rw_lock.unlock();

  // Add and sleep so that reader
  // has a chance to read
  REQUIRE(q.Add(k_pi));
  sleep(1);

  rw_lock.lock();
  num_write = 1;
  if (num_read != 1) {
    cerr << "WaitRemove seemingly doesn't notice the Addition of";
    cerr << "a string to the queue. Possible deadlock or not implemented";
    cerr << endl;
  }
  REQUIRE(1 == num_read);
  rw_lock.unlock();

  // next, test the case where there are already
  // values in the queue when the reader calls WaitRemove.
  rw_lock.lock();
  REQUIRE(q.Add(k_one));
  REQUIRE(q.Add(k_rs));
  REQUIRE(q.Add(k_pi));
  q.Close();
  REQUIRE_FALSE(q.Add(k_negative));
  REQUIRE_FALSE(q.Add(k_zero));
  num_write += 3;
  rw_lock.unlock();

  // sleep for a bit to give a chance for reader to start 
  // and to try and WaitRemove from q
  sleep(3);

  int secs_waited;
  for (secs_waited = 3; secs_waited < 30; secs_waited += 3) {
    rw_lock.lock();
    if (num_read == 5) {
      rw_lock.unlock();
      break;
    }
    rw_lock.unlock();
    sleep(3);
  }

  // make sure that we didn't time out
  // waiting for the reader thread to
  // read 3 more times.
  REQUIRE(secs_waited < 30);
  
  // start testing the second q 
  rw_lock.lock();
  REQUIRE(second.Add(k_rs));
  num_read = 0;
  num_write = 1;
  rw_lock.unlock();

  sleep(5);

  // check that the second queue actually read what we sent, and
  // should be blocked on WaitRemove now.
  // if it all looks good, Close it and wait for the child.
  rw_lock.lock();
  REQUIRE(num_read == 1);
  second.Close();
  REQUIRE_FALSE(second.Add(k_negative));
  num_write = 2;
  rw_lock.unlock();

  // sleep for a bit to give a chance for reader to unblock
  // from WaitRemove after q is Closed.
  sleep(3);

  for (secs_waited = 3; secs_waited < 30; secs_waited += 3) {
    rw_lock.lock();
    if (num_read == -1) {
      rw_lock.unlock();
      break;
    }
    rw_lock.unlock();
    sleep(3);
  }

  // make sure that we didn't time out
  // waiting for the reader thread to
  // wake up after queue is Closed
  REQUIRE(secs_waited < 30);

  reader.join();

  // make sure the reader had no errors
  REQUIRE(reader_result);
}

void read_strings(MessageQueue<string>& queue, int& num_read, int& num_write, std::mutex& rw_lock, bool& reader_result) {
  optional<string> value;

  rw_lock.lock();
  num_read += 1;
  rw_lock.unlock();

  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_pi) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_pi << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  while (true) {
    rw_lock.lock();
    if (num_write == 5) {
      rw_lock.unlock();
      break;
    }
    rw_lock.unlock();
    sleep(1);
  }


  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_one) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_one << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_negative) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_negative << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_zero) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_zero << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_rs) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_rs << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();
}

static void read_closed(MessageQueue<string>& queue, MessageQueue<string>& second, int& num_read, int& num_write, std::mutex& rw_lock, bool& reader_result) {
  optional<string> value;

  // should get it once fine from calling WaitRemove
  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_pi) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_pi << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  // loop till num_write is 4, to indicate that parent has written 3 more things
  // and Closed the queue.
  while (true) {
    rw_lock.lock();
    if (num_write == 4) {
      rw_lock.unlock();
      break;
    }
    rw_lock.unlock();
    sleep(1);
  }

  // at this point, the queue should be Closed but there are three values in it.
  // get one with WaitRemove, another with Remove and the last with WaitRemove.
  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_one) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_one << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  value = queue.Remove();
  rw_lock.lock();
  if (!value.has_value() || value != k_rs) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_rs << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  value = queue.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_pi) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_pi << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;
  rw_lock.unlock();

  // should get nullopt when we call WaitRemove in the future since it is now Closed
  for (int i = 0; i < 3; i++) {
    value = queue.WaitRemove();
    rw_lock.lock();
    if (value.has_value()) {
      reader_result = false;
      cerr << "Incorrect value from returned from call to WaitRemove()";
      cerr << "from the reader thread" << endl;
      cerr << "\tExpected: nullopt" << endl;
      cerr << "\tActual: ";
      if (value.has_value()) {
        cerr << value.value() << endl;
      } else {
        cerr << "nullopt" << endl;
      }
    }
    rw_lock.unlock();
  }

  // should get nullopt when we call Remove in the future since it is now Closed
  for (int i = 0; i < 3; i++) {
    value = queue.Remove();
    rw_lock.lock();
    if (value.has_value()) {
      reader_result = false;
      cerr << "Incorrect value from returned from call to Remove()";
      cerr << "from the reader thread" << endl;
      cerr << "\tExpected: nullopt" << endl;
      cerr << "\tActual: ";
      if (value.has_value()) {
        cerr << value.value() << endl;
      } else {
        cerr << "nullopt" << endl;
      }
    }
    rw_lock.unlock();
  }

  // test a few more cases with the second queue
  
  // increment num_read to indicate we are ready for the next queues
  rw_lock.lock();
  num_read += 1;
  rw_lock.unlock();

  // get a value from the queue fine
  value = second.WaitRemove();
  rw_lock.lock();
  if (!value.has_value() || value != k_rs) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: " << k_rs << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  num_read += 1;

  // ensure only 1 value has been written so far
  // for this test we count "Close" as a write
  if (num_write != 1) {
    cerr << "TEST ERROR: value of num_write should be 1, parent should not have written to queue yet." << endl;
    cerr << "Contact Travis unless you modified the tests" << endl;
    rw_lock.unlock();
    reader_result = false;
    return;
  }

  // call Remove on an empty (but not Closed) queue and get nullopt
  value = second.Remove();

  if (value.has_value()) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to wait()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: nullopt" << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }
  rw_lock.unlock();

  // call WaitRemove and block since the queue is empty (but not Closed)
  // we should get woken up by the call to Close()
  // in the producer thread.
  value = second.WaitRemove();

  rw_lock.lock();
  if (value.has_value()) {
    reader_result = false;
    cerr << "Incorrect value from returned from call to WaitRemove()";
    cerr << "from the reader thread" << endl;
    cerr << "\tExpected: nullopt" << endl;
    cerr << "\tActual: ";
    if (value.has_value()) {
      cerr << value.value() << endl;
    } else {
      cerr << "nullopt" << endl;
    }
  }

  if (num_write != 2) {
    cerr << "child returned from WaitRemove without parent calling Close." << endl;
    reader_result = false;
  }

  // set this num_read to -1 to indicate that we have terminated.
  num_read = -1;
  rw_lock.unlock();
}
