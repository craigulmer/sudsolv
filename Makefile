

all: sudsolv


sudsolv: sudsolv.c
	gcc -O2 sudsolv.c -o $@ -lm

clean:
	rm -f sudsolv
