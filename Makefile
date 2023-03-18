all: proctopk.c threadtopk.c
	gcc -Wall -g -o proctopk proctopk.c -lrt
	gcc -Wall -g -o threadtopk threadtopk.c -lpthread

clean:
	rm -fr threadtopk *~ output*
	rm -fr proctopk *~ output*