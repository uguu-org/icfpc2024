CFLAGS = -march=native -O3 -Wall -pedantic -std=c++20

all: a.exe lambdaman.exe spaceship.exe 3d.exe

a.exe: main.o eval.o
	g++ $(CFLAGS) $^ -o $@

main.o: main.cc eval.h
	g++ $(CFLAGS) -c $< -o $@

eval.o: eval.cc eval.h
	g++ $(CFLAGS) -c $< -o $@

lambdaman.exe: lambdaman.cc
	g++ $(CFLAGS) $< -o $@

spaceship.exe: spaceship.cc
	g++ $(CFLAGS) $< -o $@

3d.exe: 3d.cc
	g++ $(CFLAGS) $< -lgmpxx -lgmp -o $@

test: test_passed.eval

test_passed.eval: a.exe test_eval.sh
	./test_eval.sh $< && touch $@

clean:
	-rm -f *.exe *.o test_passed.*
