mfs: mfs.c loop.c
	gcc -o msh mfs.c
	gcc -o loop loop.c
clean:
	rm msh