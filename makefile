CFLAGS=-O3 -std=c++1z
CFLAGS+= -g

LIBS=-lm

minmax_dist:
	g++ $(CFLAGS) minmax_dist.cpp -o minmax_dist $(LIBS)

tester:
	g++ $(CFLAGS) tester.cpp -o tester $(LIBS) -DFAST_CORESET

clean:
	rm minmax_dist tester