all: clean exec

exec: cg.c
	g++ -o cg cg.c -lglut -lGLU -lGL -lm

clean:
	rm -f *.o	