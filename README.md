# Library Loan Concurrent System

This project implements a concurrent library loan system using MPI (Message Passing Interface) in C. The system simulates a distributed environment where multiple borrowers and libraries interact concurrently to borrow, lend, and donate books. The project is designed for educational purposes, demonstrating distributed algorithms, leader election, and message passing in a parallel computing context.

### Project Structure

- **coordinator.c**: The main program that initializes the MPI environment, coordinates the simulation, and manages the distributed processes (coordinator, libraries, borrowers).
- **list_lib.c**: Contains the implementation of linked list data structures and operations for managing books in libraries, books borrowed by borrowers, and extra book donations.
- **header.h**: Header file with type definitions, enumerations for message types, and function prototypes.
- **Makefile**: Build and run instructions for different MPI environments (OpenMPI, MPICH).
- **testfiles_hy486/**: Contains test input files and images for simulation scenarios.

### How It Works

The system consists of three main types of processes:

1. **Coordinator**: Rank 0 in MPI. Reads input files, starts and terminates the simulation, and coordinates the overall process.
2. **Libraries**: Each library manages a collection of books, handles lending, receiving donations, and participates in leader election among libraries.
3. **Borrowers**: Each borrower can borrow books, return them, and participate in leader election among borrowers.

Processes communicate using custom MPI messages defined in the `MessagePack` struct. The simulation supports events such as borrowing books, donating books, and querying the most popular book.

### Data Structures

The project uses several custom linked list data structures to manage books and their states:

- **BookLibraries**: Represents a book in a library. Each node contains:
	- `b_id`: Book ID
	- `l_id`: Library ID where the book is located
	- `cost`: Cost of the book
	- `loaned`: Whether the book is currently loaned out
	- `times_loaned`: Number of times the book has been loaned
	- `copies`: Number of copies available
	- `next`: Pointer to the next book in the list

- **BookBorrowers**: Represents a book borrowed by a borrower. Each node contains:
	- `b_id`: Book ID
	- `c_id`: Borrower (client) ID
	- `cost`: Cost of the book
	- `borrowed`: Whether the book is currently borrowed
	- `times_borrowed`: Number of times the book has been borrowed by this borrower
	- `next`: Pointer to the next borrowed book

- **ExtraBookDonations**: Represents extra books donated to libraries. Each node contains:
	- `b_id`: Book ID
	- `l_id`: Library ID where the book is donated
	- `next`: Pointer to the next donation

These data structures are managed using standard linked list operations (insert, delete, search, print) implemented in `list_lib.c`. They allow efficient tracking and updating of the state of books across the distributed system.

### Building and Running

To build the project, use:

```bash
make
```

To run the simulation locally with OpenMPI (replace N with the desired parameter):

```bash
make run-local-openmpi N=<number>
```

To run with MPICH:

```bash
make run-local-mpich N=<number>
```

To run with a hostfile (for distributed environments):

```bash
make run-mpi N=<number>
```

