mfs: mfs.c
	gcc -o msh mfs.c
	./msh
clean:
	rm msh