CXX = g++ -std=c++11

CFLAGS := -g -W -Wall -Wno-unused-parameter -I. -O$(O)

ifndef disable-gmp
	CFLAGS += -DGMP
endif

LIBS := -L/usr/local/lib -lgmp -lgmpxx -L../src/ -lcorels -Wl,-R../src/

tests: test_cases/* tests-main.cpp
	$(CXX) $(CFLAGS) $^ -o $@ $(LIBS)

clean: 
	rm tests
