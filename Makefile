.PHONY = clean all tidy-check format

HPP_SOURCE_FILES = parallel_algorithm.hpp MessageQueue.hpp
IPP_SOURCE_FILES = parallel_algorithm.ipp MessageQueue.ipp

all: test_suite

test_suite: test_suite.o catch.o test_messagequeue.o test_parallel_algorithm.o
	clang++-19 -g3 -gdwarf-4 -pthread --std=c++2b -o test_suite test_suite.o catch.o test_messagequeue.o test_parallel_algorithm.o

test_messagequeue.o: test_messagequeue.cpp catch.hpp MessageQueue.hpp MessageQueue.ipp
	clang++-19 -g3 -gdwarf-4 -pthread --std=c++2b -c test_messagequeue.cpp
	
test_parallel_algorithm.o: test_parallel_algorithm.cpp catch.hpp parallel_algorithm.hpp parallel_algorithm.ipp
	clang++-19 -g3 -gdwarf-4 -pthread --std=c++2b -c test_parallel_algorithm.cpp

test_suite.o: catch.hpp test_suite.cpp
	clang++-19 -g3 -gdwarf-4 -pthread --std=c++2b -c test_suite.cpp

catch.o: catch.hpp catch.cpp
	clang++-19 -g3 -gdwarf-4 -pthread --std=c++2b -c catch.cpp


clean:
	rm *.o test_suite

tidy-check: 
	clang-tidy-19 \
        --extra-arg=--std=c++2b \
        -warnings-as-errors=* \
        -header-filter=.* \
        $(HPP_SOURCE_FILES)

format:
	clang-format-19 -i --verbose --style=Chromium $(HPP_SOURCE_FILES) $(IPP_SOURCE_FILES)
