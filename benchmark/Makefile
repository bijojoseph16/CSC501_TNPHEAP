all: benchmark validate

benchmark: benchmark.c 
	$(CC) -g -O0 benchmark.c -o benchmark -I/usr/local/include -lnpheap -ltnpheap
	
validate: validate.c 
	$(CC) -g -O0 validate.c -o validate -lnpheap -ltnpheap
	
clean:
	rm -f benchmark validate tests 
