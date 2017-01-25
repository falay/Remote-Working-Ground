all:
	make -C RAS
	g++ -std=c++14 -O3 -c  RWG.cpp Main.cpp
	g++ -std=c++14 -O3  RWG.o Main.o RAS/RAS.o RAS/Pipe.o -o RWGserver
	
clean:
	rm -rf *.o RWGserver	
	make clean -C RAS