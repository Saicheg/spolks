NAME=mpi-collective

CC=mpicc
CFLAGS=-ggdb -std=gnu99 -Wall

LIBS=-lm

MPI_FLAGS=-n 4
MPI_ARGS=

default: build

run: build
	@./mpirun $(MPI_FLAGS) $(NAME) $(MPI_ARGS)

build: clean
	@echo Building...
	@$(CC) $(CFLAGS) -o $(NAME) *.c $(LIBS)

clean:
	@echo Cleaning...
	@rm -f $(NAME) $(NAME).o
