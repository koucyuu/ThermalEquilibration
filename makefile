CFLAGS = -Wall -g -Wno-deprecated-declarations -Wno-implicit-function-declaration -framework OpenGL -framework GLUT

OBJS = mdv

all: $(OBJS)

clean:
	@rm -f $(OBJS)
	@rm -rf *.dSYM