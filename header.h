#ifndef HEADER_H
#define HEADER_H
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define READY_TAG 1
typedef enum
{
    READY,
    TERMINATE,
    AWAKE,
    /* Messages for creating lenders' tree (begin) */
    CONNECT,
    ACK,
    NEIGHBOR,
    /* Messages for creating lenders' tree (end) */
    /* Messages for electing DFS leader for libraries (begin) */
    START_LEADER_ELECTION,
    LEADER,
    PARENT,
    ALREADY,
    KNOW_YOUR_LEADER,
    LE_LIBR_DONE,
    /* Messages for electing DFS leader for libraries (end) */
    /* Messages for electing DFS leader for borrowers (begin) */
    START_LE_LOANERS,
    ELECT,
    LE_LOANERS,
    KNOW_YOUR_LEADER_BORROWER,
    LEADER_CONFIRM,
    LE_LOANERS_DONE,
    /* Messages for electing DFS leader for borrowers (end) */
    /* Messages for TakeBook event (begin) */
    WANT_BORROW_BOOK,
    LEND_BOOK,
    FIND_BOOK,
    GET_BOOK,
    BOOK_REQUEST,
    FOUND_CORRECT_L_ID,
    ACK_TB,
    DONE_FIND_BOOK,
    /* Messages for TakeBook event (end) */
    /* Messages for DonateBook event (begin) */
    DONATE_BOOKS,
    DONATE_BOOK,
    DONATE_SINGLE_BOOK,
    ACK_DB,
    DONATION_BOOKS_DONE,
    UPDATE_LEADER_NEW_B_ID,
    LEADER_UPDATED_FOR_NEW_B_ID,
    /* Messages for DonateBook event (end) */
    /* Messages for GetMostPopuralBook event (begin) */
    GET_MOST_POPULAR_BOOK,
    B_MAX,
    GET_POPULAR_BK_INFO,
    ACK_BK_INFO,
    GET_MOST_POPULAR_BOOK_DONE,
    /* Messages for GetMostPopuralBook event (end) */
    /* Messages for CheckNumBooksLoaned (begin) */
    CHECK_NUM_BOOKS_LOAN,
    NUM_BOOKS_LOANED,
    ACK_NBL,
    CHECK_NUM_BOOKS_LOAN_DONE
    /* Messages for CheckNumBooksLoaned (end) */
} Messages_E;

typedef struct BookLibraries
{
    int b_id;
    int l_id;
    int cost;
    int loaned;
    int times_loaned;
    int copies;
    struct BookLibraries* next;
} BookLibraries;

typedef struct BookBorrowers
{
    int b_id;
    int c_id;
    int cost;
    int borrowed;
    int times_borrowed;
    struct BookBorrowers* next;
} BookBorrowers;

typedef struct ExtraBookDonations
{
    int b_id;
    int l_id;
    struct ExtraBookDonations* next;
} ExtraBookDonations;

/* Tight packing*/
#pragma pack(push, 1)
typedef struct
{
    int message_type;
    int neighbor_to_become;
    int leader_id;
    int leader_rank;
    int b_id;
    int correct_l_id;
    int c_id;
    int l_id_source;
    int n_copies;
    int count_borrowed;
    int cost;
    int initial_l_id_requester;
} MessagePack;
#pragma pack(pop)

BookLibraries* insert_library_book(BookLibraries* head, int b_id, int cost, int copies, int l_id);
BookLibraries* delete_library_book(BookLibraries* head, int b_id);
BookLibraries* search_library_book(BookLibraries* head, int b_id);
void print_library_books(BookLibraries* head);

BookBorrowers* insert_borrower_book(BookBorrowers* head, int b_id, int c_id, int cost);
BookBorrowers* delete_borrower_book(BookBorrowers* head, int b_id);
BookBorrowers* search_borrower_book(BookBorrowers* head, int b_id);
BookBorrowers* find_max_borrowed(BookBorrowers* head);
void print_borrower_books(BookBorrowers* head);

ExtraBookDonations* insert_extra_book(ExtraBookDonations* head, int b_id, int l_id);
ExtraBookDonations* delete_extra_book(ExtraBookDonations* head, int b_id, int l_id);
ExtraBookDonations* search_extra_book(ExtraBookDonations* head, int b_id);
void print_extra_books(ExtraBookDonations* head);

#endif
