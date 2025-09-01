#include "header.h"

/* Library Books Operations */
BookLibraries* insert_library_book(BookLibraries* head, int b_id, int cost, int copies, int l_id)
{
    BookLibraries* new_book = (BookLibraries*)malloc(sizeof(BookLibraries));
    if (new_book == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    new_book->b_id = b_id;
    new_book->cost = cost;
    new_book->loaned = 0;
    new_book->times_loaned = 0;
    new_book->next = head; 
    new_book->copies = copies;
    new_book->l_id = l_id;
    return new_book;
}

BookLibraries* delete_library_book(BookLibraries* head, int b_id)
{
    BookLibraries* current = head;
    BookLibraries* previous = NULL;

    while (current != NULL)
    {
        if (current->b_id == b_id)
        {
            if (previous == NULL)
            {
                head = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current);
            printf("Book %d deleted from library\n", b_id);
            return head;
        }
        previous = current;
        current = current->next;
    }

    printf("Book %d not found in library\n", b_id);
    return head;
}

BookLibraries* search_library_book(BookLibraries* head, int b_id)
{
    BookLibraries* current = head;
    while (current != NULL)
    {
        if (current->b_id == b_id)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_library_books(BookLibraries* head)
{
    if (head == NULL)
    {
        return;
    }
    BookLibraries* current = head;
    while (current != NULL)
    {
        printf("l_id: %d, Book ID: %d, Copies: %d, Cost: %d, Loaned: %s, Times Loaned: %d\n", current->l_id, current->b_id, current->copies, current->cost, current->loaned ? "Yes" : "No",
               current->times_loaned);
        fflush(stdout);
        current = current->next;
    }
}

/* Borrower Books Operations */
BookBorrowers* insert_borrower_book(BookBorrowers* head, int b_id, int c_id, int cost)
{
    BookBorrowers* new_book = (BookBorrowers*)malloc(sizeof(BookBorrowers));
    if (new_book == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    new_book->b_id = b_id;
    new_book->borrowed = 1;       
    new_book->times_borrowed = 1; 
    new_book->next = head;        
    new_book->c_id = c_id;
    new_book->cost = cost;
    return new_book;
}

BookBorrowers* delete_borrower_book(BookBorrowers* head, int b_id)
{
    BookBorrowers* current = head;
    BookBorrowers* previous = NULL;

    while (current != NULL)
    {
        if (current->b_id == b_id)
        {
            if (previous == NULL)
            {
                head = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current);
            printf("Book %d deleted from borrower's list\n", b_id);
            return head;
        }
        previous = current;
        current = current->next;
    }
    return head;
}

BookBorrowers* search_borrower_book(BookBorrowers* head, int b_id)
{
    BookBorrowers* current = head;
    while (current != NULL)
    {
        if (current->b_id == b_id)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_borrower_books(BookBorrowers* head)
{
    BookBorrowers* current = head;

    while (current != NULL)
    {
        printf("c_id: %d, Book ID: %d, Cost: %d, Borrowed: %s, Times Borrowed: %d\n", current->c_id, current->b_id, current->cost, current->borrowed ? "Yes" : "No", current->times_borrowed);
        fflush(stdout);
        current = current->next;
    }
}

BookBorrowers* find_max_borrowed(BookBorrowers* head)
{
    BookBorrowers* max = head;
    BookBorrowers* current = head;
    while (current != NULL)
    {
        if (current->times_borrowed > max->times_borrowed)
        {
            max = current;
        }
        current = current->next;
    }
    return max;
}

/* Extra Donation Books Operations */
ExtraBookDonations* insert_extra_book(ExtraBookDonations* head, int b_id, int l_id)
{
    ExtraBookDonations* new_book = (ExtraBookDonations*)malloc(sizeof(ExtraBookDonations));
    if (new_book == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    new_book->b_id = b_id;
    new_book->l_id = l_id;
    new_book->next = head;
    return new_book;
}

ExtraBookDonations* delete_extra_book(ExtraBookDonations* head, int b_id, int l_id)
{
    ExtraBookDonations* current = head;
    ExtraBookDonations* previous = NULL;

    while (current != NULL)
    {
        if (current->b_id == b_id && current->l_id == l_id)
        {
            if (previous == NULL)
            {
                head = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current);
            return head;
        }
        previous = current;
        current = current->next;
    }
    return head;
}

ExtraBookDonations* search_extra_book(ExtraBookDonations* head, int b_id)
{
    ExtraBookDonations* current = head;
    while (current != NULL)
    {
        if (current->b_id == b_id)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void print_extra_books(ExtraBookDonations* head)
{
    ExtraBookDonations* current = head;

    while (current != NULL)
    {
        printf("Book ID: %d, Located In l_id: %d\n", current->b_id, current->l_id);
        fflush(stdout);
        current = current->next;
    }
}