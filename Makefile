CC=mpicc
CFLAGS= -Wall -O0
HOSTFILE= myhosts.txt

all: coordinator

coordinator: coordinator.o list_lib.o
	$(CC) $(CFLAGS) -o coordinator coordinator.o list_lib.o

coordinator.o: coordinator.c header.h
	$(CC) $(CFLAGS) -c coordinator.c

list_lib.o: list_lib.c header.h
	$(CC) $(CFLAGS) -c list_lib.c

run-local-openmpi: all
	@if [ -z "$(N)" ]; then \
		echo "Usage: make run-local N=<number>"; \
	else \
		COORD_NP=1; \
		BORROW_NP=$$(expr $(N) \* $(N) \* $(N) / 2); \
		if [ $$BORROW_NP -lt 1 ]; then BORROW_NP=1; fi; \
		LIB_NP=$$(expr $(N) \* $(N)); \
		NP=$$(expr $$COORD_NP + $$BORROW_NP + $$LIB_NP); \
		echo "Running with $$NP processes (Coordinator:1, Borrowers:$$BORROW_NP, Libraries:$$LIB_NP)"; \
		mpirun --oversubscribe -np $$NP ./coordinator $(N); \
	fi

run-local-mpich: all
	@if [ -z "$(N)" ]; then \
		echo "Usage: make run-local N=<number>"; \
	else \
		COORD_NP=1; \
		BORROW_NP=$$(expr $(N) \* $(N) \* $(N) / 2); \
		if [ $$BORROW_NP -lt 1 ]; then BORROW_NP=1; fi; \
		LIB_NP=$$(expr $(N) \* $(N)); \
		NP=$$(expr $$COORD_NP + $$BORROW_NP + $$LIB_NP); \
		echo "Running with $$NP processes (Coordinator:1, Borrowers:$$BORROW_NP, Libraries:$$LIB_NP)"; \
		mpirun -np $$NP ./coordinator $(N); \
	fi

run-mpi: all
	@if [ -z "$(N)" ]; then \
		echo "Usage: make run-mpi N=<number>"; \
	else \
		COORD_NP=1; \
		BORROW_NP=$$(expr $(N) \* $(N) \* $(N) / 2); \
		if [ $$BORROW_NP -lt 1 ]; then BORROW_NP=1; fi; \
		LIB_NP=$$(expr $(N) \* $(N)); \
		NP=$$(expr $$COORD_NP + $$BORROW_NP + $$LIB_NP); \
		echo "Running with $$NP processes (Coordinator:1, Borrowers:$$BORROW_NP, Libraries:$$LIB_NP)"; \
		mpirun --hostfile $(HOSTFILE) -np $$NP ./coordinator $(N); \
	fi

clean:
	rm -f coordinator.o list_lib.o coordinator