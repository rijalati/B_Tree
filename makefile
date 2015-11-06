all: example_creation example_tester

example_tester: example_tester.o b_tree.o jdisk.o
	gcc -o example_tester example_tester.o b_tree.o jdisk.o

example_tester.o: example_tester.c
	gcc -c example_tester.c

example_creation: example_creation.o b_tree.o jdisk.o
	gcc -o example_creation example_creation.o b_tree.o jdisk.o

example_creation.o: example_creation.c b_tree.h
	gcc -c example_creation.c

b_tree.o: b_tree.c 
	gcc -c b_tree.c

jdisk.o: jdisk.h
	gcc -c jdisk.c
