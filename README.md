# hw8

CIT 5950 (Spring 2026)    HW 08: Multithreaded Data Sharing
Threads & Synchronization in C++.

Goals
In completing this assignment, you will learn how to:

Share data between threads in a program
Solve a more complex parallel program using threads
Collaboration
For assignments in CIT 5950, you will complete each of them on your own. However, you may discuss high-level ideas with other students, but any viewing, sharing, copying, or dictating of code is forbidden. If you are worried about whether something violates academic integrity, please post on Ed or contact the instructor(s).

Contents
Setup
Overview
Instructions
Suggested Order
Grading and Testing
Compilation
Valgrind
Helgrind
testing
Catch2
Submission
Setup
For this assignment, you need to setup a Linux C++ development environment. You can use the same environment as the one used in the previous assignment. The instructions are here if you need them: Enivronment setup

You can download the starter files into your docker container by running the following command:

curl -o parallel_starter.zip https://www.seas.upenn.edu/~cit5950/current/projects/code/parallel_starter.zip
You can also download the files manually here if you would like: parallel_starter.zip

From here, you need to extract the files by running

unzip parallel_starter.zip
From here you can either open the project in Vim or VSCode.

Overview
 You must use use C++ threads, specifically std::jthread. You are not allowed to use pthread.

 You should not use any global variables for any of the code you write in this assignment.

MessageQueue
In class we talked about the producer/consumer problem where one or more threads produce some thing and then one or more other threads consume those things. To solve this problem, some sort of MessageQueue is often used. Notably this has to be some sort of container to hold multiple values since many values may be produced before consumers start consuming them.

In this part you will implement a datastructure to handle the transfer of information from one thread to another.

This data structure will be called a MessageQueue. See the Instructions section below for details on the code files and how you should write some programs to do this

Parallel Algorithms
In class we talked about parallel implementations of the higher order functions map (called transform in this assignment) and reduce. In this part of the assignment you will be implementing these functions in C++ with a few changes.

One thing that is different from lecture is the number of threads we create. In lecture we created an arbitrary number of threads as needed based on the input size. In this assignment you will use std::thread::hardware_concurrency() which returns the number of threads that can run at a time on your machine. Note that this function can return 0, and if it does for your machine then instead use 4 as the value. e.g.

size_t num_threads = std::thread::hardware_concurrency();
num_threads = num_threads == 0 ? 4 : num_threads;
Important thing to add onto this! you should not create this many threads! If std::threads::hardware_concurrency() returns 12 then you should only create 11 threads. This is because we already have a thread! The thread calling our functoin! This thread should also do part of the work involved in the algorithm , not just create other threads to do the work.

Note that there are some inputs where we want to create less than std::threads::hardware_conurrency() - 1 threads. Some times we want to create 0 additional threads! If the size of the input is small, then you should only utilize as many threads as we could benefit from. Consider calling trasform on a input vector of lenght 1. Well, there isn’t a way to break that work up across multiple threads, so 0 threads are created. only the already existing calling thread is used to do work.

HINT: try to think crticially about what is the minimal amount of inputs needed to do some work. It is a different number for transform and for reduce.

 You should write your code to roughly follow how they were demonstrated to work in class. Check the lecture slides for more details.

Instructions
The description of what you are implementing in each part of the assignment can be found in their respective .hpp files. These are MessageQueue.hpp and parallel_algorithm.hpp. If you have questions on them then please ask on Ed discussion!

Note that for both parts of the assignment, we are writing template code so that the code can be applied to different types. Therefore we are writing our code in a .ipp file that the .hpp file will #include at the bottom. (You can see this yourself if you open the .hpp files). The ipp files are included at the bottom of hpp files so that your .ipp code is effectively “in the header”, which is necessary for how templates work. Templates are instantiated (generated into assembly) when they are used, not when they are declared. Because of this the code we write in our ipp files must follow a similar style to code we write in a .hpp file. For us this means that you cannot use using namespace std; or anything like using std::jthread;. Doing using statements like this is bad practice in .hpp files since then the using statement would apply to any file that #include’s the hpp file. So you must use the full name for types: std::vector<T>, std::thread::hardware_concurrency(), etc.

Other than the .ipp files, you will have to also modity the MessageQueue.hpp file to add data members to the MessageQueue object. You should not need to modify anything else in the .hpp files, but you are free to add other things if it would help. Do keep in mind that the tests expect the same interface for your object and functions as we initially provide.

You MUST use the the C++ standard library for threads and synchronization. Sepecifically:

std::jthread
std::mutex
std::scoped_lock
std::unique_lock
std::condition_variable
We have provided a test_suite for both parts of the assignment so that you can verify the implementation works correctly but you may want to run most of the tests in test_suite under helgrind to make sure they use threads correctly. (See the helgrind section below for which tests not to run on helgrind).

Note that to have MessageQueue be thread safe, you MUST use a std::mutex in your data structure. It is up to you to decide when to use it. You should also use a std::condition_variable to get the solution working.

Your parallel_algorithm code should not need the use of any mutex or condition variable since you split the work up across threads without overlap. The work should be split up roughly evenly, but if some threads do more work than others, that is ok as long as it is roughly split evenly.

Suggested Order
Your job is to complete both parts of the assignment: MessageQueue and parallel_algorithms. However you can do each part independently. Start whichever part you like. The only suggestions we have on the order are:

For MessageQueue:

First add some data membres to MessageQueue.hpp
Implement everything other than wait_remove and make sure you can pass the [MessageQueue] Add_Remove tests
implement the rest so that you can pass all [MessageQueue] tests.
For parallel_algoritm:

finish transform() first and make sure you pass all the [transform] tests.
finish reduce() first and make sure you pass all the [reduce] tests.
Grading and Testing
Compilation
We have supplied you with a Makefile that can be used for compiling your code into an executable.

You may need to resolve any compiler warnings and compiler errors that show up. Once all compiler errors have been resolved, if you ls in the terminal, you should be able to see an executable called test_suite listed.

How to run these executables is described below.

Note that your submission will be partially evaluated on the number of compiler warnings. You should eliminate ALL compiler warnings in your code.

Valgrind
We will also test your submission on whether there are any memory errors or memory leaks. We will be using valgrind to do this. To do this, you should try running:

valgrind --leak-check=full ./test_suite

If everything is correct, you should see the following towards the bottom of the output:

 ==1620== All heap blocks were freed -- no leaks are possible
 ==1620==
 ==1620== For counts of detected and suppressed errors, rerun with: -v
 ==1620== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
If you do not see something similar to the above in your output, valgrind will have printed out details about where the errors and memory leaks occurred.

Helgrind
We will also test your submission on whether there are any thread errors. We will be using the tool helgrind to do this, which is a tool under valgrind To do this, you should try running something like: valgrind --tool=helgrind ./test_suite

If everything is correct, you should see the following towards the bottom of the output:

==14004== For counts of detected and suppressed errors, rerun with: -v
==14004== Use --history-level=approx or =none to gain increased speed, at
==14004== the cost of reduced accuracy of conflicting-access information
==14004== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 216 from 42)
Your output may look slightly different, but the most important thing here is that it says 0 errors from 0 contexts.

 Some of the tests (notably [reduce] uses_threads) may have false positives and identify things that are not actually issues. Your helgrind should pass for all other tests. You can pass arguments to catch2 to specify which tests to run so you can avoid running the test that has issues. For example:

valgrind --tool=helgrind ./test_suite [reduce] int_sum
Testing
Catch2
As with hw0, you can compile the your implementation by using the make command. This will result in several output files, including an executable called test_suite.

After compiling your solution with make, You can run all of the tests for MessageQueue and parallel_algorithm by invoking:

./test_suite
You can also run only specific tests by passing command line arguments into test_suite

./test_suite [MessageQueue] Add_Remove
You can specify which tests are run for any of the tests in the assingment. You just need to know the names of the tests, and you can do this by running:

$ ./test_suite --list-tests
These settings can be helpful for debugging specific parts of the assignment, especially since test_suite can be run with these settings through valgrind and gdb!

Code Quality
For this assignment, we will be checking the quality of your code in two ways:

Through automated checks with clang-format & clang-tidy
Through manually reviewing your code for any style issues.
Note that there is a lot less room to make style choices in this code, so it is weighted less. We do the check stil to make sure you are following good practices and that your code is following the specification.

Reminder to check our code quality document to see what we are looking for in your code.

Submission:
Please submit your completed MessageQueue.hpp, MessageQueue.ipp, parallel_algorithm.hpp, and parallel_algorithm.ipp Gradescope via your github repo. We will be checking for compilation warnings, valgrind errors, making sure the test_suite runs correctly, and that you did not violate any rules established in the specification.

Note: In addition to the autograder, we will be manually reviewing the submissions for this homework assignment to ensure that you are using threads and that you are using them as described in the write-up. Just because you get full points on the test suite does not mean that you will get full points after we review your submissions manually.

Note: It is expected to take a while for this to run.
