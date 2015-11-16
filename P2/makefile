P2: P2.o P2Helpers.o P2MessageDrivers.o
	gcc -g -o P2 P2.o P2Helpers.o P2MessageDrivers.o
	rm *.o

P2.o: P2.c
	gcc -g -o P2.o -c P2.c

P2Helpers.o: P2Helpers.c
	gcc -g -o P2Helpers.o -c P2Helpers.c

P2MessageDrivers.o: P2MessageDrivers.c
	gcc -g -o P2MessageDrivers.o -c P2MessageDrivers.c

.Phony: clean

clean:
	rm *.o
	rm P2
