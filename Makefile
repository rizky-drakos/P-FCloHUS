exe: main.o  utils.o models.o
	g++ -O3 -march=native -o exe main.o  utils.o models.o -fopenmp

main.o: src/main.cpp
	g++ -O3 -march=native -c src/main.cpp -fopenmp

utils.o: src/utils.cpp
	g++ -O3 -march=native -c src/utils.cpp -fopenmp

models.o: src/models.cpp
	g++ -O3 -march=native -c src/models.cpp -fopenmp

clean:
	rm -f *.o exe

# -g -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2 -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector
# -O3 -march=native -mtune=native