#include "header.h"
MPI_Datatype MessagePackType;
/* For TakeBook Event */
int c_id_2_l_id(int c_id, int N) { return (c_id - (N * N)) / (N / 2) % (N * N); }
/* Check B_IDS THIS IS WRONG */
int global_to_local_l_id(int global_id, int N) { return global_id / (N * N); }
int global_to_local_index_inside_library(int global_id, int N) { return global_id % N; }
int local_indexes_to_b_id(int l_id, int index_inside_library, int N) { return l_id * N + index_inside_library; }
/* General translation */
int c_id_2_rank(int c_id) { return c_id + 1; }
int l_id_2_rank(int l_id, int N)
{
    int y = l_id / N;
    int x = l_id % N;
    return 1 + N * x + y;
}
int rank_2_c_id(int rank) { return rank - 1; }
void create_mpi_types(MPI_Datatype* MessagePackType)
{
    // --- MessagePack ---
    int lengths1[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Aint displacements1[12];
    MessagePack dummy1 = {.message_type = 0,
                          .neighbor_to_become = 0,
                          .leader_id = 0,
                          .leader_rank = 0,
                          .b_id = 0,
                          .correct_l_id = 0,
                          .c_id = 0,
                          .l_id_source = 0,
                          .n_copies = 0,
                          .count_borrowed = 0,
                          .cost = 0,
                          .initial_l_id_requester = 0};
    MPI_Aint base_address;
    MPI_Get_address(&dummy1, &base_address);
    MPI_Get_address(&dummy1.message_type, &displacements1[0]);
    MPI_Get_address(&dummy1.neighbor_to_become, &displacements1[1]);
    MPI_Get_address(&dummy1.leader_id, &displacements1[2]);
    MPI_Get_address(&dummy1.leader_rank, &displacements1[3]);
    MPI_Get_address(&dummy1.b_id, &displacements1[4]);
    MPI_Get_address(&dummy1.correct_l_id, &displacements1[5]);
    MPI_Get_address(&dummy1.c_id, &displacements1[6]);
    MPI_Get_address(&dummy1.l_id_source, &displacements1[7]);
    MPI_Get_address(&dummy1.n_copies, &displacements1[8]);
    MPI_Get_address(&dummy1.count_borrowed, &displacements1[9]);
    MPI_Get_address(&dummy1.cost, &displacements1[10]);
    MPI_Get_address(&dummy1.initial_l_id_requester, &displacements1[11]);

    displacements1[0] -= base_address;
    displacements1[1] -= base_address;
    displacements1[2] -= base_address;
    displacements1[3] -= base_address;
    displacements1[4] -= base_address;
    displacements1[5] -= base_address;
    displacements1[6] -= base_address;
    displacements1[7] -= base_address;
    displacements1[8] -= base_address;
    displacements1[9] -= base_address;
    displacements1[10] -= base_address;
    displacements1[11] -= base_address;

    MPI_Datatype types1[12] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Type_create_struct(12, lengths1, displacements1, types1, MessagePackType);
    MPI_Type_commit(MessagePackType);
}

/* ----------------------------------------------------------- LIBRARY CODE --------------------------------------------------------- */

typedef struct
{
    int parent;
    int leader_elected;
    int leader_id;
    int* children;
    int unexplored[4];
    int num_children;
    int l_id;
} DFSLibraryAttributes;

void explore(DFSLibraryAttributes* attributes, int N)
{
    /* Find first unexplored */
    for (int i = 0; i < 4; i++)
    {
        if (attributes->unexplored[i] != -1)
        {
            /* let pk be a library in unexplored */
            /* remove pk from unexplored */
            /* send <leader, leader> to pk */
            int l_id_2_send = attributes->unexplored[i];
            attributes->unexplored[i] = -1;
            MessagePack leader_message_2_send;
            leader_message_2_send.message_type = LEADER;
            leader_message_2_send.leader_id = attributes->leader_id;
            MPI_Send(&leader_message_2_send, 1, MessagePackType, l_id_2_rank(l_id_2_send, N), 0, MPI_COMM_WORLD);
            return;
        }
    }
    /* If we didn't find any unexplored children */
    if (attributes->parent != attributes->l_id)
    {
        MessagePack parent_message_2_send;
        parent_message_2_send.message_type = PARENT;
        parent_message_2_send.leader_id = attributes->leader_id;
        // printf("provlhma an einai -1: %d\n", attributes->parent);
        // fflush(stdout);
        MPI_Send(&parent_message_2_send, 1, MessagePackType, l_id_2_rank(attributes->parent, N), 0, MPI_COMM_WORLD);
    }
    else /* We are over (we are the leader) send the leader elected to the coordinator */
    {

        /* Before sending done to coordinator make sure every node in tree knows the leader */
        // printf("Leader %d sending KNOW_YOUR_LEADER to all\n", attributes->l_id);
        // fflush(stdout);
        for (int i = 0; i < attributes->num_children; i++)
        {

            MessagePack leader_query;
            leader_query.message_type = KNOW_YOUR_LEADER;
            leader_query.leader_id = attributes->l_id;
            // printf("Leader %d sending KNOW_YOUR_LEADER to all\n", attributes->l_id);
            // fflush(stdout);
            MPI_Send(&leader_query, 1, MessagePackType, l_id_2_rank(attributes->children[i], N), 0, MPI_COMM_WORLD);
        }
    }
}

/* Returns the l_id closest to the target l_id */
int get_next_lid_towards_target(int N, int current_l_id, int dest_l_id)
{
    int curr_x = current_l_id % N;
    int curr_y = current_l_id / N;
    int dest_x = dest_l_id % N;
    int dest_y = dest_l_id / N;

    if (curr_y < dest_y)
    {
        return (curr_y + 1) * N + curr_x;
    }
    else if (curr_y > dest_y)
    {
        return (curr_y - 1) * N + curr_x;
    }
    else if (curr_x < dest_x)
    {
        return curr_y * N + (curr_x + 1);
    }
    else if (curr_x > dest_x)
    {
        return curr_y * N + (curr_x - 1);
    }

    return current_l_id;
}

void library_handler(int rank, int N)
{
    int x, y;         // Grid coordinates
    int neighbors[4]; // [left, right, up, down], -1 if no neighbor
    int l_id;
    BookLibraries* books = NULL;
    ExtraBookDonations* extra_donations_locations = NULL;
    DFSLibraryAttributes tree_node_details;
    // printf("started library with %d rank\n", rank);
    // fflush(stdout);
    x = (rank - 1) / N;
    y = (rank - 1) % N;
    /* Coordinates y is the column i is the line but we use them as inverse (l_ids are stored) */
    neighbors[0] = (x > 0) ? y * N + (x - 1) : -1;     // left
    neighbors[1] = (x < N - 1) ? y * N + (x + 1) : -1; // right
    neighbors[2] = (y > 0) ? (y - 1) * N + x : -1;     // up
    neighbors[3] = (y < N - 1) ? (y + 1) * N + x : -1; // down
    l_id = (N * y + x);
    MessagePack awake;
    awake.message_type = AWAKE;
    MPI_Send(&awake, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
    // printf("o l_id: %d stelnei awake\n", l_id);
    // fflush(stdout);
    /* Init tree attributes */
    tree_node_details.parent = -1;
    tree_node_details.leader_elected = 0;
    tree_node_details.leader_id = -1;
    tree_node_details.children = NULL;
    tree_node_details.num_children = 0;
    tree_node_details.l_id = l_id;
    for (int i = 0; i < 4; i++)
    {
        /* At the beginning all the neighbors are considered unexplored (-1 will mean its explored) */
        tree_node_details.unexplored[i] = neighbors[i];
    }
    /* Book list implementation */
    for (int i = 0; i < N; i++)
    {
        int b_id = ((l_id * N) + i);
        int cost = (rand() % 96) + 5;
        int copies = N;
        // printf("BIDS GENERATED %d FOR L_ID %d\n", b_id, l_id);
        // fflush(stdout);
        books = insert_library_book(books, b_id, cost, copies, l_id);
    }

    // print_library_books(books);

    // printf("x: %d y:%d, l_id: %d, rank: %d, neighbors: %d %d %d %d, l_id_2_rank: %d\n", x, y, l_id, rank, neighbors[0], neighbors[1], neighbors[2], neighbors[3], l_id_2_rank(l_id, N));
    // fflush(stdout);
    MPI_Status status_message;
    int online = 1;
    while (online)
    {
        MessagePack message_received;
        /* Receive any type of message in the air */
        /* int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status) */
        MPI_Recv(&message_received, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status_message);
        /* Now check its type and see what this message is */
        /* upon receiving no message I will attempt to create a spanning tree with leader as my self */
        if (message_received.message_type == START_LEADER_ELECTION)
        {
            // printf("library to elave\n");
            // fflush(stdout);
            if (tree_node_details.parent == -1)
            {
                tree_node_details.leader_id = l_id;
                tree_node_details.parent = l_id;
                for (int i = 0; i < 4; i++)
                {
                    tree_node_details.unexplored[i] = neighbors[i];
                }
                explore(&tree_node_details, N);
            }
        }
        /* upon receiving <leader, new_id> from pj */
        else if (message_received.message_type == LEADER)
        {
            /* If the new leader_id is bigger now i belong to the new tree with greated ID */
            if (tree_node_details.leader_id < message_received.leader_id)
            {
                tree_node_details.leader_id = message_received.leader_id;
                int x = (status_message.MPI_SOURCE - 1) / N;
                int y = (status_message.MPI_SOURCE - 1) % N;
                int source_l_id = (N * y + x);
                tree_node_details.parent = source_l_id;
                tree_node_details.num_children = 0;
                tree_node_details.children = NULL;
                /* Remove pj from unexplored since he came here acting as the leader */
                for (int i = 0; i < 4; i++)
                {
                    if (tree_node_details.unexplored[i] == source_l_id)
                    {
                        tree_node_details.unexplored[i] = -1;
                    }
                    else
                    {
                        tree_node_details.unexplored[i] = neighbors[i];
                    }
                }
                explore(&tree_node_details, N);
            }
            else if (tree_node_details.leader_id == message_received.leader_id)
            {
                MessagePack already_message_2_send;
                already_message_2_send.message_type = ALREADY;
                already_message_2_send.leader_id = tree_node_details.leader_id;
                int x = (status_message.MPI_SOURCE - 1) / N;
                int y = (status_message.MPI_SOURCE - 1) % N;
                int source_l_id = (N * y + x);
                MPI_Send(&already_message_2_send, 1, MessagePackType, l_id_2_rank(source_l_id, N), 0, MPI_COMM_WORLD);
            }
        }
        /* upon receiving <already, new_id> from pj */
        else if (message_received.message_type == ALREADY)
        {
            if (message_received.leader_id == tree_node_details.leader_id)
            {
                explore(&tree_node_details, N);
            }
        }
        /* upon receiving <parent, new_id> from pj */
        else if (message_received.message_type == PARENT)
        {
            if (message_received.leader_id == tree_node_details.leader_id)
            {
                tree_node_details.num_children++;
                tree_node_details.children = realloc(tree_node_details.children, tree_node_details.num_children * sizeof(int));
                int x = (status_message.MPI_SOURCE - 1) / N;
                int y = (status_message.MPI_SOURCE - 1) % N;
                int source_l_id = (N * y + x);
                tree_node_details.children[tree_node_details.num_children - 1] = source_l_id;
                explore(&tree_node_details, N);
            }
        }
        /* upon receiving <KNOW_YOUR_LEADER> to send confirm to new_leader */
        else if (message_received.message_type == KNOW_YOUR_LEADER)
        {
            // printf("o l_id %d kserei ton leader %d \n", l_id, tree_node_details.leader_id);
            tree_node_details.leader_id = message_received.leader_id;
            fflush(stdout);
            /* Redistribute the broadcast down */
            for (int i = 0; i < tree_node_details.num_children; i++)
            {

                MessagePack leader_query;
                leader_query.message_type = KNOW_YOUR_LEADER;
                leader_query.leader_id = tree_node_details.leader_id;
                // printf("Intermediate %d sending KNOW_YOUR_LEADER to all\n", tree_node_details.l_id);
                // fflush(stdout);
                MPI_Send(&leader_query, 1, MessagePackType, l_id_2_rank(tree_node_details.children[i], N), 0, MPI_COMM_WORLD);
            }
            /* Start the convergecast if you are a leaf */
            if (tree_node_details.num_children == 0)
            {
                MessagePack confirm;
                confirm.message_type = LEADER_CONFIRM;
                confirm.leader_id = tree_node_details.leader_id;
                MPI_Send(&confirm, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
                // printf("stelnw ston parent pleon oti ton kserw %d\n", tree_node_details.parent);
                // fflush(stdout);
            }
        }
        else if (message_received.message_type == LEADER_CONFIRM)
        {
            /* If leader */
            if (tree_node_details.l_id == tree_node_details.leader_id)
            {
                static int nodes_confirming = 0;
                nodes_confirming++;
                // printf("Leader received confirmation %d/%d\n", nodes_confirming, tree_node_details.num_children);
                // fflush(stdout);
                if (nodes_confirming >= tree_node_details.num_children)
                {
                    MessagePack library_leader_done;
                    library_leader_done.message_type = LE_LIBR_DONE;
                    library_leader_done.leader_id = l_id;
                    MPI_Send(&library_leader_done, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
                    // printf("Leader election complete, sent done to coordinator\n");
                    // fflush(stdout);
                    nodes_confirming = 0;
                }
            }
            /* General case */
            else
            {
                static int nodes_confirming = 0;
                nodes_confirming++;
                // printf("Parent received confirmation %d/%d\n", nodes_confirming, tree_node_details.num_children);
                // fflush(stdout);
                if (nodes_confirming >= tree_node_details.num_children)
                {
                    MessagePack library_leader_done;
                    library_leader_done.message_type = LEADER_CONFIRM;
                    library_leader_done.leader_id = l_id;
                    MPI_Send(&library_leader_done, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
                    // printf("Leader election done for this parea send to parent\n");
                    // fflush(stdout);
                    nodes_confirming = 0;
                }
            }
        }
        /* TakeBook event */
        /* Initial library receives message if it finds it the first try it returns GET_BOOK */
        else if (message_received.message_type == LEND_BOOK)
        {
            /* Try to find it here */
            BookLibraries* target_book = search_library_book(books, message_received.b_id);
            MessagePack message_to_respond = message_received;
            if (target_book != NULL)
            {
                if (target_book->copies > 0)
                {
                    target_book->loaned = 1;
                    target_book->times_loaned++;
                    target_book->copies--;
                }
                else
                {
                    message_to_respond.b_id = -1;
                }
                message_to_respond.message_type = GET_BOOK;
                message_to_respond.cost = target_book->cost;
                message_to_respond.l_id_source = l_id;
                message_to_respond.initial_l_id_requester = l_id;
                // printf("first tryyyyyyy\n");
                // fflush(stdout);
                MPI_Send(&message_to_respond, 1, MessagePackType, c_id_2_rank(message_received.c_id), 0, MPI_COMM_WORLD);
            }
            else
            {
                /* We didn't find it interrupt the leader */
                if (tree_node_details.leader_id == l_id)
                {
                    /* If we are already the leader just perform the appropriate things to spot the book */
                    // printf("o leader eimai egw kai voh8iemai exw l_id%d, b_id na vrw%d, c_id gia debug:%d\n", l_id, message_received.b_id, message_received.c_id);
                    // fflush(stdout);
                    /* Leader has access to N^4 array which contains all the global id_s and the libraries where they belong */
                    int correct_l_id = -1; /* It means that the book was not found */

                    /* Check where we should look the b_id on the extra donations list, or on the N^4 arrays */
                    if (message_received.b_id > (N * N * N) - 1)
                    {
                        ExtraBookDonations* target_book = search_extra_book(extra_donations_locations, message_received.b_id);
                        if (target_book != NULL)
                        {
                            correct_l_id = target_book->l_id;
                            extra_donations_locations = delete_extra_book(extra_donations_locations, target_book->b_id, target_book->l_id);
                        }
                        else
                        {
                            // printf("----------------YPARXEI PROVLHMA DEN VRE8HKE TO VIVLIO STHN EXTRA LIST-----\n");
                            // fflush(stdout);
                            correct_l_id = -1;
                        }
                    }
                    else
                    {
                        /* Initialize the mapping (every library knows its own books) */
                        int* book_to_library_map = (int*)malloc(N * N * N * N * sizeof(int));
                        int b_id_counter = 0;
                        for (int i = 0; i < N * N * N * N; i++)
                        {
                            if (i % N == 0 && i != 0) /* Every N copies increment the b_id [000,111,222], [333,444,555] */
                            {
                                b_id_counter++;
                            }
                            book_to_library_map[i] = b_id_counter;
                        }
                        /* Find the b_id you look for */
                        for (int i = 0; i < N * N * N * N; i++)
                        {
                            if (message_received.b_id == book_to_library_map[i]) /* Every N copies increment the b_id [000,111,222], [333,444,555] */
                            {
                                correct_l_id = i / (N * N);
                                break;
                            }
                        }
                    }

                    /* Time to do the request to the proper library (do it immediately) */
                    MessagePack proper_request_to_correct_library = message_received;
                    proper_request_to_correct_library.message_type = BOOK_REQUEST;
                    proper_request_to_correct_library.correct_l_id = correct_l_id;
                    proper_request_to_correct_library.initial_l_id_requester = l_id;
                    MPI_Send(&proper_request_to_correct_library, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, correct_l_id), N), 0, MPI_COMM_WORLD);
                }
                else
                {

                    message_to_respond.message_type = FIND_BOOK;
                    message_to_respond.initial_l_id_requester = l_id;
                    // printf("stelnw oti 8elw voh8eia pros ton leader %d kai to twrino mou l_id einai %d\n", tree_node_details.leader_id, l_id);
                    // fflush(stdout);
                    MPI_Send(&message_to_respond, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
                }
            }
        }
        /* Leader receives the FIND_BOOK message */
        else if (message_received.message_type == FIND_BOOK)
        {
            if (l_id == tree_node_details.leader_id)
            {
                // printf("o leader kalesthke gia voh8eia exw l_id%d, b_id na vrw%d, c_id gia debug:%d prepei na epistrepsw sthn arxikh vivlio8hkh %d\n", l_id, message_received.b_id,
                //        message_received.c_id, message_received.initial_l_id_requester);
                // fflush(stdout);
                /* Leader has access to N^4 array which contains all the global id_s and the libraries where they belong */
                int correct_l_id = -1; /* It means that the book was not found */
                /* Check where we should look the b_id on the extra donations list, or on the N^4 arrays */
                if (message_received.b_id > (N * N * N) - 1)
                {
                    ExtraBookDonations* target_book = search_extra_book(extra_donations_locations, message_received.b_id);
                    if (target_book != NULL)
                    {
                        correct_l_id = target_book->l_id;
                        extra_donations_locations = delete_extra_book(extra_donations_locations, target_book->b_id, target_book->l_id);
                    }
                    else
                    {
                        // printf("----------------YPARXEI PROVLHMA DEN VRE8HKE TO VIVLIO STHN EXTRA LIST-----\n");
                        // fflush(stdout);
                        correct_l_id = -1;
                    }
                }
                else
                {
                    /* Initialize the mapping (every library knows its own books) */
                    int* book_to_library_map = (int*)malloc(N * N * N * N * sizeof(int));
                    int b_id_counter = 0;
                    for (int i = 0; i < N * N * N * N; i++)
                    {
                        if (i % N == 0 && i != 0) /* Every N copies increment the b_id [000,111,222], [333,444,555] */
                        {
                            b_id_counter++;
                        }
                        book_to_library_map[i] = b_id_counter;
                    }
                    /* Find the b_id you look for */
                    for (int i = 0; i < N * N * N * N; i++)
                    {
                        if (message_received.b_id == book_to_library_map[i]) /* Every N copies increment the b_id [000,111,222], [333,444,555] */
                        {
                            correct_l_id = i / (N * N);
                            break;
                        }
                    }
                }
                MessagePack inform_borrower_for_correct_l_id = message_received;
                inform_borrower_for_correct_l_id.message_type = FOUND_CORRECT_L_ID;
                inform_borrower_for_correct_l_id.correct_l_id = correct_l_id;
                // printf("to c_id apo kei pou phra to request einai %d\n", message_received.c_id);
                // fflush(stdout);
                MPI_Send(&inform_borrower_for_correct_l_id, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.initial_l_id_requester), N), 0, MPI_COMM_WORLD);
            }
            else
            {
                /* Forward the message up to the leader */
                MessagePack forward_message_up_2_leader = message_received;
                forward_message_up_2_leader.message_type = FIND_BOOK;
                MPI_Send(&forward_message_up_2_leader, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == FOUND_CORRECT_L_ID) /* This could be wrong this is the normal case */
        {
            /* Initial library receives that leader has found correct_l_id */
            if (l_id == message_received.initial_l_id_requester)
            {
                /* Time to do the request to the proper library only if we found the book if the correct l_id is not -1 */
                if (message_received.correct_l_id != -1)
                {
                    MessagePack proper_request_to_correct_library = message_received;
                    proper_request_to_correct_library.message_type = BOOK_REQUEST;
                    MPI_Send(&proper_request_to_correct_library, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.correct_l_id), N), 0, MPI_COMM_WORLD);
                }
                else
                {
                    MessagePack send_not_found_back_to_borrower = message_received;
                    send_not_found_back_to_borrower.message_type = ACK_TB;
                    send_not_found_back_to_borrower.b_id = -1;
                    MPI_Send(&send_not_found_back_to_borrower, 1, MessagePackType, c_id_2_rank(message_received.c_id), 0, MPI_COMM_WORLD);
                }
            }
            else
            {
                /* Need to go closer to initial requester and forward it to him */
                MessagePack inform_library_for_correct_l_id = message_received;
                inform_library_for_correct_l_id.message_type = FOUND_CORRECT_L_ID;
                // printf("eimai o %d 8a paw ston %d\n", l_id, get_next_lid_towards_target(N, l_id, message_received.initial_l_id_requester));
                MPI_Send(&inform_library_for_correct_l_id, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.initial_l_id_requester), N), 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == BOOK_REQUEST)
        {
            /* Now the actual library who has the book receives the message */
            if (l_id == message_received.correct_l_id)
            {
                // printf("me vrhkan kai eimai h swsth vivlio8hkh %d\n", l_id);
                // fflush(stdout);
                /* Find the book in the correct library now */
                BookLibraries* target_book = search_library_book(books, message_received.b_id);
                if (target_book == NULL)
                {
                    /* This must never happen because if we dont find the book we look for then we just return ACK_TB to initial borrower */
                    // printf("---------------------------MEGALO PROVLHMA 'H EGINE DONATE GIA TO VIVLIO POU PSAXNOUME ---------------------------\n");
                }
                else
                {
                    MessagePack send_ack_initial_library = message_received;
                    if (target_book->copies > 0)
                    {
                        /* Now that the book was found update its things */
                        target_book->loaned = 1;
                        target_book->times_loaned++;
                        target_book->copies--;
                        send_ack_initial_library.b_id = message_received.b_id;
                    }
                    else
                    {
                        /* If no copies left return -1 as the b_id to the initial l_id requester */
                        // printf("NOOOOOOOOOO COPIES %d \n", message_received.b_id);
                        // fflush(stdout);
                        send_ack_initial_library.b_id = -1;
                    }
                    /* Send back an ACK_TB to initial library */
                    send_ack_initial_library.message_type = ACK_TB;
                    send_ack_initial_library.cost = target_book->cost;
                    send_ack_initial_library.l_id_source = l_id;
                    MPI_Send(&send_ack_initial_library, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.initial_l_id_requester), N), 0, MPI_COMM_WORLD);
                }
            }
            else
            {
                /* Need to forward closer to the correct library */
                MessagePack inform_correct_library = message_received;
                // printf("eimai o %d 8a paw ston %d\n", l_id, get_next_lid_towards_target(N, l_id, message_received.initial_l_id_requester));
                MPI_Send(&inform_correct_library, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.correct_l_id), N), 0, MPI_COMM_WORLD);
            }
        }
        /* Initial library receives ACK_TB */
        else if (message_received.message_type == ACK_TB)
        {
            if (message_received.initial_l_id_requester == l_id)
            {
                // printf("shmainei oti egine \n");
                // fflush(stdout);
                MessagePack send_ack_to_borrower = message_received;
                send_ack_to_borrower.message_type = ACK_TB;
                MPI_Send(&send_ack_to_borrower, 1, MessagePackType, c_id_2_rank(send_ack_to_borrower.c_id), 0, MPI_COMM_WORLD);
            }
            else
            {
                MessagePack forward_ack_to_initial_library = message_received;
                MPI_Send(&forward_ack_to_initial_library, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.initial_l_id_requester), N), 0, MPI_COMM_WORLD);
            }
        }
        /* DonateBook event */
        else if (message_received.message_type == DONATE_SINGLE_BOOK)
        {
            /* First try to find the book and increment its things if it doesn't exit insert it */
            BookLibraries* target_book = search_library_book(books, message_received.b_id);
            if (target_book == NULL)
            {
                books = insert_library_book(books, message_received.b_id, (rand() % 96) + 5, 1, l_id);
            }
            else
            {
                target_book->copies++;
            }
            /* It doesnt make a lot of sense because the donated books are received from the borrower leader not the l_id but... */
            printf("Received donate book %d from %d\n", message_received.b_id, l_id);
            fflush(stdout);
            /* Also if the b_id we just store is >= N^3 that means that this b_id wasn't in any library initially,
            so we need to send a message to the leader in order for him to store this book in which library it was stored */
            if (message_received.b_id > (N * N * N) - 1)
            {
                /* If we are already the rank just insert else send a message to inform */
                if (rank == l_id_2_rank(tree_node_details.leader_id, N))
                {
                    // printf("ton vazw monos mouuuuuuuuuuuuuuuuuuuuuuuuuuuuuu\n");
                    // fflush(stdout);
                    extra_donations_locations = insert_extra_book(extra_donations_locations, message_received.b_id, tree_node_details.leader_id);
                }
                else
                {
                    // printf("enhmerwnwn ton leader twra epishs\n");
                    // fflush(stdout);
                    MessagePack update_leader_new_b_id = message_received;
                    update_leader_new_b_id.message_type = UPDATE_LEADER_NEW_B_ID;
                    update_leader_new_b_id.b_id = message_received.b_id;
                    update_leader_new_b_id.l_id_source = l_id; /* This is the l_id the new library was placed so we need to store that the new_book b_id was stored in l_id */
                    MPI_Send(&update_leader_new_b_id, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
                    MessagePack leader_got_updated_for_new_b_id;
                    MPI_Recv(&leader_got_updated_for_new_b_id, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
            MessagePack ack_insert_done = message_received;
            ack_insert_done.message_type = ACK_DB;
            // printf("stelnw to ack moy\n");
            // fflush(stdout);
            MPI_Send(&ack_insert_done, 1, MessagePackType, status_message.MPI_SOURCE, 0, MPI_COMM_WORLD);
        }
        else if (message_received.message_type == UPDATE_LEADER_NEW_B_ID)
        {
            if (l_id == tree_node_details.leader_id)
            {
                // printf("o leader enhmerw8hke kai evale to neo b_id sthn lista newn\n");
                // fflush(stdout);
                /* Insert into the new list the b_id (the newlly added one) and its library also its num of copies */
                extra_donations_locations = insert_extra_book(extra_donations_locations, message_received.b_id, message_received.l_id_source);
                MessagePack update_done_2_l_id_source = message_received;
                update_done_2_l_id_source.message_type = LEADER_UPDATED_FOR_NEW_B_ID;
                MPI_Send(&update_done_2_l_id_source, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.l_id_source), N), 0, MPI_COMM_WORLD);
            }
            else
            {
                MessagePack forward_new_message_2_leader = message_received;
                MPI_Send(&forward_new_message_2_leader, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == LEADER_UPDATED_FOR_NEW_B_ID)
        {
            /* Update from leader to l_id that received the donation that the update got done */
            MessagePack update_done_2_l_id_source = message_received;
            MPI_Send(&update_done_2_l_id_source, 1, MessagePackType, l_id_2_rank(get_next_lid_towards_target(N, l_id, message_received.l_id_source), N), 0, MPI_COMM_WORLD);
        }
        /* CheckNumBooksLoaned event */
        else if (message_received.message_type == CHECK_NUM_BOOKS_LOAN)
        {
            // printf("h library %d to ema8e\n", l_id);
            // fflush(stdout);

            /* Count loaned books (this should happen for ALL libraries) */
            int loaned_count = 0;
            BookLibraries* traversal = books;
            while (traversal != NULL)
            {
                loaned_count = loaned_count + traversal->times_loaned;
                traversal = traversal->next;
            }
            // printf("loaned count is %d for l_id%d\n", loaned_count, l_id);
            // fflush(stdout);

            /* Send to leader our loaned count only if we aren't the leader */
            if (l_id != tree_node_details.leader_id)
            {
                MessagePack message_loaned_count_2_leader = message_received;
                message_loaned_count_2_leader.message_type = NUM_BOOKS_LOANED;
                message_loaned_count_2_leader.count_borrowed = loaned_count;
                message_loaned_count_2_leader.l_id_source = l_id; /* Source */
                MPI_Send(&message_loaned_count_2_leader, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
                /* Now wait for ack before continuing */
                MessagePack wait_for_ack_from_leader;
                MPI_Recv(&wait_for_ack_from_leader, 1, MessagePackType, l_id_2_rank(tree_node_details.leader_id, N), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // printf("ELAVA TOSA 8ETIKA ACK APO LEADER\n");
                // fflush(stdout);
            }

            /* Rest of your existing forwarding logic... */
            if (l_id != 0 && status_message.MPI_SOURCE == 0)
            {
                MessagePack message_2_send_2_next = message_received;
                message_2_send_2_next.message_type = CHECK_NUM_BOOKS_LOAN;
                message_2_send_2_next.l_id_source = l_id;
                MPI_Send(&message_2_send_2_next, 1, MessagePackType, l_id_2_rank(0, N), 0, MPI_COMM_WORLD);
            }
            else if (l_id == 0 && l_id == tree_node_details.leader_id)
            {
                MessagePack message_2_send_2_next = message_received;
                message_2_send_2_next.l_id_source = l_id;
                if (l_id + 1 < N * N)
                {
                    MPI_Send(&message_2_send_2_next, 1, MessagePackType, l_id_2_rank(l_id + 1, N), 0, MPI_COMM_WORLD);
                }
            }
            else
            {
                MessagePack message_2_send_2_next = message_received;
                message_2_send_2_next.l_id_source = l_id;
                if (l_id + 1 < N * N)
                {
                    MPI_Send(&message_2_send_2_next, 1, MessagePackType, l_id_2_rank(l_id + 1, N), 0, MPI_COMM_WORLD);
                }
            }
        }
        else if (message_received.message_type == NUM_BOOKS_LOANED)
        {
            if (tree_node_details.leader_id == l_id)
            {
                static int count_received = 0;
                static int total_loans = 0;
                count_received++;
                total_loans = total_loans + message_received.count_borrowed;
                /* Also send ACK to source so he can move on */
                MessagePack ack_2_source = message_received;
                ack_2_source.message_type = ACK_NBL;
                MPI_Send(&ack_2_source, 1, MessagePackType, l_id_2_rank(message_received.l_id_source, N), 0, MPI_COMM_WORLD);
                /* I received from everyone its loaned count so send the total one to coordinator */
                if (count_received == (N * N) - 1)
                {
                    /* Add the leader details too */
                    BookLibraries* traversal = books;
                    while (traversal != NULL)
                    {
                        total_loans = total_loans + traversal->times_loaned;
                        traversal = traversal->next;
                    }
                    MessagePack message_2_coord = message_received;
                    message_2_coord.message_type = CHECK_NUM_BOOKS_LOAN_DONE;
                    message_2_coord.count_borrowed = total_loans;
                    printf("Library Books %d\n", total_loans);
                    fflush(stdout);
                    MPI_Send(&message_2_coord, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
                    count_received = 0;
                    total_loans = 0;
                }
            }
            else
            {
                MessagePack books_loaned_info = message_received;
                books_loaned_info.message_type = NUM_BOOKS_LOANED;
                MPI_Send(&books_loaned_info, 1, MessagePackType, l_id_2_rank(tree_node_details.parent, N), 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == TERMINATE)
        {
            // printf("l_id:%d and knows leader:%d has parent:%d and %d children: ", l_id, tree_node_details.leader_id, tree_node_details.parent, tree_node_details.num_children);
            // for (int i = 0; i < tree_node_details.num_children; i++)
            // {
            //     printf("%d ", tree_node_details.children[i]);
            // }
            // printf("\n");
            // fflush(stdout);
            // print_library_books(books);
            // print_extra_books(extra_donations_locations);
            online = 0;
        }
    }
    return;
}
/* ----------------------------------------------------------- BORROWER CODE --------------------------------------------------------- */
typedef struct
{
    int* neighbors;
    int num_neighbors;
    int* received_neighbors; /* 0 means not received,1 means received */
    int sent_elect;
    int count_received;
    int leader;
    int broadcasted;
    int parent;
} STBorrowerAttributes;

int neighbor_exists(int* neighbors, int num_neighbors, int possible_new)
{
    for (int i = 0; i < num_neighbors; i++)
    {
        if (possible_new == neighbors[i])
        {
            return 1;
        }
    }
    return 0;
}

void borrower_handler(int rank, int N)
{
    int c_id = rank - 1;
    // printf("started borrower with c_id: %d\n", c_id);
    // fflush(stdout);
    BookBorrowers* books = NULL;
    BookBorrowers* max_books = NULL;
    STBorrowerAttributes tree_node_details;
    tree_node_details.num_neighbors = 0;
    tree_node_details.neighbors = NULL; // Tree neighbors
    tree_node_details.received_neighbors = NULL;
    tree_node_details.sent_elect = 0;
    tree_node_details.count_received = 0;
    tree_node_details.leader = -1;
    tree_node_details.broadcasted = 0;
    tree_node_details.parent = -1;
    // Signal coordinator that this borrower is ready
    MessagePack awake;
    awake.message_type = AWAKE;
    MPI_Send(&awake, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
    // printf("o c_id: %d stelnei awake\n", c_id);
    // fflush(stdout);
    MPI_Status status_message;
    int online = 1;
    while (online)
    {
        MessagePack message_received;
        /* Receive any type of message in the air */
        /* int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status) */
        MPI_Recv(&message_received, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status_message);
        /* Now check its type and see what this message is */
        if (message_received.message_type == CONNECT)
        {
            /* Now I need to update neighbors etc... */
            /* Check this neighbor doesnt already exist because for a <> b , b <> a connections on test */
            if (neighbor_exists(tree_node_details.neighbors, tree_node_details.num_neighbors, message_received.neighbor_to_become) == 0)
            {
                tree_node_details.num_neighbors++;
                tree_node_details.neighbors = realloc(tree_node_details.neighbors, tree_node_details.num_neighbors * sizeof(int));
                tree_node_details.neighbors[tree_node_details.num_neighbors - 1] = message_received.neighbor_to_become;
                /* Also update the received to contain 0 */
                tree_node_details.received_neighbors = realloc(tree_node_details.received_neighbors, tree_node_details.num_neighbors * sizeof(int));
                tree_node_details.received_neighbors[tree_node_details.num_neighbors - 1] = 0;
            }
            /* id1 was updated now I need the other borrower too that I'm his neighbor */
            MessagePack connect_message_2_send;
            connect_message_2_send.message_type = NEIGHBOR;
            connect_message_2_send.neighbor_to_become = c_id;
            /* int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) */
            MPI_Send(&connect_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[tree_node_details.num_neighbors - 1]), 0, MPI_COMM_WORLD);
            /* Wait ACK from id2 who is your new neighbor */
            MessagePack ack_message_received;
            /* int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status) */
            MPI_Recv(&ack_message_received, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[tree_node_details.num_neighbors - 1]), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            /* Now that we have received ACK we need to send back ACK to coordinator */
            MessagePack ack_message_2_send;
            ack_message_2_send.message_type = ACK;
            MPI_Send(&ack_message_2_send, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
        }
        else if (message_received.message_type == NEIGHBOR)
        {
            /* Check this neighbor doesnt already exist because for a <> b , b <> a connections on test */
            if (neighbor_exists(tree_node_details.neighbors, tree_node_details.num_neighbors, message_received.neighbor_to_become) == 0)
            {
                tree_node_details.num_neighbors++;
                tree_node_details.neighbors = realloc(tree_node_details.neighbors, tree_node_details.num_neighbors * sizeof(int));
                tree_node_details.neighbors[tree_node_details.num_neighbors - 1] = message_received.neighbor_to_become;
                /* Also update the received to contain 0 */
                tree_node_details.received_neighbors = realloc(tree_node_details.received_neighbors, tree_node_details.num_neighbors * sizeof(int));
                tree_node_details.received_neighbors[tree_node_details.num_neighbors - 1] = 0;
            }
            MessagePack ack_message_2_send;
            ack_message_2_send.message_type = ACK;
            /* Send ACK back to the id1 who initiated you */
            MPI_Send(&ack_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[tree_node_details.num_neighbors - 1]), 0, MPI_COMM_WORLD);
        }
        /* upon receiving START_LE_LOANERS start convergecast if you are a leaf */
        else if (message_received.message_type == START_LE_LOANERS)
        {
            /* Here we start the convergecast */
            if (tree_node_details.num_neighbors == 1)
            {
                /* send ELECT to only neighbor */
                MessagePack elect_message_2_send;
                elect_message_2_send.message_type = ELECT;
                MPI_Send(&elect_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[0]), 0, MPI_COMM_WORLD);
                tree_node_details.sent_elect = 1;
            }
        }
        /* upon receiving ELECT from MPI_SOURCE (pj) */
        else if (message_received.message_type == ELECT)
        {
            tree_node_details.count_received++;
            /* Find the c_id in the neighbors array and make received[i] = 1 */
            for (int i = 0; i < tree_node_details.num_neighbors; i++)
            {
                if (tree_node_details.neighbors[i] == rank_2_c_id(status_message.MPI_SOURCE))
                {
                    tree_node_details.received_neighbors[i] = 1;
                    break;
                }
            }
            /* If we have received <ELECT> from all neighbors before we have sent any ELECT we are the leader */
            if (tree_node_details.count_received == tree_node_details.num_neighbors && tree_node_details.sent_elect == 0)
            {
                /* We are the leader start Broadcasting */
                tree_node_details.leader = rank;
                MessagePack leader_message_2_send;
                leader_message_2_send = message_received;
                leader_message_2_send.message_type = LE_LOANERS;
                leader_message_2_send.leader_rank = rank;
                for (int i = 0; i < tree_node_details.num_neighbors; i++)
                {
                    MPI_Send(&leader_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[i]), 0, MPI_COMM_WORLD);
                }
            }
            /* Received elect from everyone except one neighbor send him <ELECT> message */
            else if (tree_node_details.count_received == tree_node_details.num_neighbors - 1 && tree_node_details.sent_elect == 0)
            {
                /* Need to send Elect to the only neighbor who hasnt sent me an ELECT message */
                /* First find the only neighbor who hasnt sent me ELECT */
                for (int i = 0; i < tree_node_details.num_neighbors; i++)
                {
                    if (tree_node_details.received_neighbors[i] == 0)
                    {
                        MessagePack elect_message_2_send;
                        elect_message_2_send.message_type = ELECT;
                        MPI_Send(&elect_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[i]), 0, MPI_COMM_WORLD);
                        tree_node_details.sent_elect = 1;
                        break;
                    }
                }
            }
            else if (tree_node_details.sent_elect)
            {
                /* If the neighbor who I haven't yet received ELECT sends me ELECT and I have sent him ELECT compare the ids and if i have bigger id i will elect myself as leader */
                for (int i = 0; i < tree_node_details.num_neighbors; i++)
                {
                    if (status_message.MPI_SOURCE == c_id_2_rank(tree_node_details.neighbors[i]) && rank > status_message.MPI_SOURCE)
                    {
                        /* We are the leader start Broadcasting */
                        tree_node_details.leader = rank;
                        MessagePack leader_message_2_send;
                        leader_message_2_send.message_type = LE_LOANERS;
                        leader_message_2_send.leader_rank = rank;
                        for (int j = 0; j < tree_node_details.num_neighbors; j++)
                        {
                            MPI_Send(&leader_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[j]), 0, MPI_COMM_WORLD);
                        }
                    }
                }
            }
        }
        /* Rebroadcast the leader but do it only once thats why I use flag broadcasted :) */
        else if (message_received.message_type == LE_LOANERS && tree_node_details.broadcasted == 0)
        {
            tree_node_details.parent = rank_2_c_id(status_message.MPI_SOURCE);
            // printf("phra to broadcast kai eimai o %d kai o parent mou einai %d\n", c_id, tree_node_details.parent);
            // fflush(stdout);
            tree_node_details.broadcasted = 1;
            tree_node_details.leader = message_received.leader_rank;
            /* Continue the broadcasting */
            MessagePack leader_message_2_send;
            leader_message_2_send.message_type = LE_LOANERS;
            leader_message_2_send.leader_rank = message_received.leader_rank;
            for (int j = 0; j < tree_node_details.num_neighbors; j++)
            {
                // Don't send back to the neighbor who sent us this message
                if (tree_node_details.neighbors[j] != rank_2_c_id(status_message.MPI_SOURCE))
                {
                    MPI_Send(&leader_message_2_send, 1, MessagePackType, c_id_2_rank(tree_node_details.neighbors[j]), 0, MPI_COMM_WORLD);
                }
            }
            /* Begin the convergecast */
            if (tree_node_details.num_neighbors == 1)
            {
                // printf("ksekinane oi %d c_id\n", c_id);
                // fflush(stdout);
                MessagePack i_know_my_leader = message_received;
                i_know_my_leader.message_type = KNOW_YOUR_LEADER_BORROWER;
                MPI_Send(&i_know_my_leader, 1, MessagePackType, status_message.MPI_SOURCE, 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == KNOW_YOUR_LEADER_BORROWER)
        {
            if (tree_node_details.leader != rank)
            {
                static int count_received_know_their_leaders = 0;
                count_received_know_their_leaders++;
                // printf("o c_id %d to elave apo %d kai o parent toy c_id einai %d kai exei labei %d / %d\n", c_id, rank_2_c_id(status_message.MPI_SOURCE), tree_node_details.parent,
                //        count_received_know_their_leaders, tree_node_details.num_neighbors - 1);
                // fflush(stdout);
                if (count_received_know_their_leaders == tree_node_details.num_neighbors - 1)
                {
                    // printf("to prow8oume ston parent:%d kai kseroume leader rank%d\n", tree_node_details.parent, tree_node_details.leader);
                    // fflush(stdout);
                    MessagePack message_2_parent = message_received;
                    message_2_parent.message_type = KNOW_YOUR_LEADER_BORROWER;
                    MPI_Send(&message_2_parent, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
                }
            }
            else
            {
                static int count_received_know_their_leaders = 0;
                count_received_know_their_leaders++;
                if (count_received_know_their_leaders == tree_node_details.num_neighbors)
                {
                    MessagePack message_2_coord = message_received;
                    message_2_coord.message_type = LE_LOANERS_DONE;
                    MPI_Send(&message_2_coord, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
                }
            }
        }
        /* TakeBook event messages */
        else if (message_received.message_type == WANT_BORROW_BOOK)
        {
            /* The library were the borrower will send the request of b_id is going to be calculated with by using the c_id library assigned to borrower so we try to find it there */
            int target_l_id = c_id_2_l_id(c_id, N);
            MessagePack lend_book_message_2_send;
            lend_book_message_2_send = message_received;
            lend_book_message_2_send.message_type = LEND_BOOK;
            /* Send that to the specified library for it to search */
            // printf("to l_id poy antistoixei ston c_id%d einai %d\n", c_id, target_l_id);
            // fflush(stdout);
            MPI_Send(&lend_book_message_2_send, 1, MessagePackType, l_id_2_rank(target_l_id, N), 0, MPI_COMM_WORLD);
        }
        else if (message_received.message_type == GET_BOOK)
        {
            /* Check that the book was actually found */
            if (message_received.b_id != -1)
            {
                /* Received the book first try */
                BookBorrowers* new_book = search_borrower_book(books, message_received.b_id);
                if (new_book == NULL)
                {
                    books = insert_borrower_book(books, message_received.b_id, c_id, message_received.cost);
                    new_book = search_borrower_book(books, message_received.b_id);
                }
                else
                {
                    new_book->borrowed = 1;
                    new_book->times_borrowed++;
                }
                printf("Got book %d from %d\n", message_received.b_id, message_received.l_id_source);
                fflush(stdout);
            }
            MessagePack done_find_book_message = message_received;
            done_find_book_message.message_type = DONE_FIND_BOOK;
            MPI_Send(&done_find_book_message, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
        }
        else if (message_received.message_type == ACK_TB)
        {
            /* Check that the book was actually found */
            if (message_received.b_id != -1)
            {
                /* Update borrowers */
                BookBorrowers* borrowed_book = search_borrower_book(books, message_received.b_id);
                if (borrowed_book == NULL)
                {
                    books = insert_borrower_book(books, message_received.b_id, c_id, message_received.cost);
                    borrowed_book = search_borrower_book(books, message_received.b_id);
                }
                else
                {
                    borrowed_book->borrowed = 1;
                    borrowed_book->times_borrowed++;
                }
                printf("Got book %d from %d\n", message_received.b_id, message_received.l_id_source);
                fflush(stdout);
            }
            /* This happens at the end */
            MessagePack done_find_book_message = message_received;
            done_find_book_message.message_type = DONE_FIND_BOOK;
            MPI_Send(&done_find_book_message, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
        }
        /* DonateBook event */
        else if (message_received.message_type == DONATE_BOOKS)
        {
            /* Send DONATE_BOOK to borrower leader for him to perform the donations */
            if (tree_node_details.leader != rank)
            {
                MessagePack donation_initiation_to_leader = message_received;
                donation_initiation_to_leader.message_type = DONATE_BOOK;
                // printf("leader is %d\n", tree_node_details.leader);
                // fflush(stdout);
                MPI_Send(&donation_initiation_to_leader, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
            }
            /* Else if the message was initially first sent to the leader just perform the same thing as downstairs */
            else
            {
                // printf("c_id: %d KAI EIMAI HDH O LEADER is %d\n", c_id, rank_2_c_id(tree_node_details.leader));
                // fflush(stdout);
                static int donate_next = 0; /* Keep it not being initialized everytime */
                /* Now we distribute n_copies among all the libraries in round-robin */
                for (int i = 0; i < message_received.n_copies; i++)
                {
                    int l_id = (donate_next) % (N * N);
                    // printf("Donating book %d copy %d to library %d\n", message_received.b_id, i, l_id);
                    // fflush(stdout);
                    MessagePack single_book_donation = message_received;
                    single_book_donation.message_type = DONATE_SINGLE_BOOK;
                    MPI_Send(&single_book_donation, 1, MessagePackType, l_id_2_rank(l_id, N), 0, MPI_COMM_WORLD);
                    donate_next++;
                    /* Now wait to receive ACK_DB from library to make sure book got updated */
                    // Check
                    while (1)
                    {
                        MessagePack waiting_to_end;
                        MPI_Recv(&waiting_to_end, 1, MessagePackType, l_id_2_rank(l_id, N), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (waiting_to_end.message_type == ACK_DB)
                        {
                            break;
                        }
                    }
                }
                MessagePack send_donation_end_to_coordinator;
                send_donation_end_to_coordinator = message_received;
                send_donation_end_to_coordinator.message_type = DONATION_BOOKS_DONE;
                // printf("edw pame\n");
                // fflush(stdout);
                MPI_Send(&send_donation_end_to_coordinator, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == DONATE_BOOK)
        {
            if (tree_node_details.leader == rank)
            {
                static int donate_next = 0; /* Keep it not being initialized everytime */
                /* Now we distribute n_copies among all the libraries in round-robin */
                for (int i = 0; i < message_received.n_copies; i++)
                {
                    int l_id = (donate_next) % (N * N);
                    // printf("Donating book %d copy %d to library %d\n", message_received.b_id, i, l_id);
                    // fflush(stdout);
                    MessagePack single_book_donation = message_received;
                    single_book_donation.message_type = DONATE_SINGLE_BOOK;
                    MPI_Send(&single_book_donation, 1, MessagePackType, l_id_2_rank(l_id, N), 0, MPI_COMM_WORLD);
                    donate_next++;
                    /* Now wait to receive ACK_DB from library to make sure book got updated */
                    // Check
                    while (1)
                    {
                        MessagePack waiting_to_end;
                        MPI_Recv(&waiting_to_end, 1, MessagePackType, l_id_2_rank(l_id, N), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (waiting_to_end.message_type == ACK_DB)
                        {
                            break;
                        }
                    }
                }
                MessagePack send_donation_end_to_coordinator;
                send_donation_end_to_coordinator = message_received;
                send_donation_end_to_coordinator.message_type = DONATION_BOOKS_DONE;
                MPI_Send(&send_donation_end_to_coordinator, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
            }
            else
            {
                /* Need to forward the message DONATE_BOOK to the leader */
                MessagePack forward_donate_book_2_leader = message_received;
                forward_donate_book_2_leader.message_type = DONATE_BOOK;
                MPI_Send(&forward_donate_book_2_leader, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
            }
        }
        /* GetMostPopuralBook event */
        else if (message_received.message_type == GET_MOST_POPULAR_BOOK)
        {
            /* Leader received GET_MOST_POPULAR_BOOK message perform a broadcast on the tree */
            int received_from = -1;                    // Track who sent the message first
            received_from = status_message.MPI_SOURCE; // First sender is the parent
            for (int i = 0; i < tree_node_details.num_neighbors; i++)
            {
                int neighbor_rank = c_id_2_rank(tree_node_details.neighbors[i]);

                /* Don't send back to parent or self */
                if (neighbor_rank != received_from && neighbor_rank != rank)
                {
                    MessagePack broadcast_msg = message_received;
                    MPI_Send(&broadcast_msg, 1, MessagePackType, neighbor_rank, 0, MPI_COMM_WORLD);
                    // printf("Node %d forwarding to neighbor %d\n", rank, neighbor_rank);
                    // fflush(stdout);
                }
            }
            /* Now that we have broadcasted the message and we aren't the leader send our most borrowed book to leader */
            if (rank != tree_node_details.leader)
            {
                /* Find the most popular book */
                MessagePack popular_book_info = message_received;
                popular_book_info.message_type = GET_POPULAR_BK_INFO;
                popular_book_info.c_id = c_id;
                BookBorrowers* max_borrowed_book = find_max_borrowed(books);
                if (max_borrowed_book == NULL)
                {
                    /* No books were borrowed by this c_id */
                    popular_book_info.b_id = -1;
                }
                else
                {
                    popular_book_info.b_id = max_borrowed_book->b_id;
                    popular_book_info.count_borrowed = max_borrowed_book->times_borrowed;
                    popular_book_info.l_id_source = max_borrowed_book->b_id / N;
                    popular_book_info.cost = max_borrowed_book->cost;
                }
                MPI_Send(&popular_book_info, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
                MessagePack ack_2_receive_from_leader;
                MPI_Recv(&ack_2_receive_from_leader, 1, MessagePackType, tree_node_details.leader, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // printf("phra to ack\n");
                // fflush(stdout);
            }
        }
        else if (message_received.message_type == GET_POPULAR_BK_INFO)
        {
            if (tree_node_details.leader == rank)
            {
                static int count_received = 0;
                count_received++;
                if (message_received.b_id != -1) /* If we received -1 it means there no books borrowed by the source borrower */
                {
                    max_books = insert_borrower_book(max_books, message_received.b_id, rank_2_c_id(status_message.MPI_SOURCE), message_received.cost);
                }
                // printf("tosa mhnymata lambanei o leader gia plhrofories\n");
                // fflush(stdout);
                MessagePack ack_2_send_2_leader = message_received;
                ack_2_send_2_leader.message_type = ACK_BK_INFO;
                MPI_Send(&ack_2_send_2_leader, 1, MessagePackType, c_id_2_rank(message_received.c_id), 0, MPI_COMM_WORLD);
                /* This means we received message from every borrower now we also need to include the leader max book */
                if (count_received == floor((N * N * N) / 2) - 1)
                {
                    /* Include leader max book */
                    BookBorrowers* leader_max_book = find_max_borrowed(books);
                    if (leader_max_book != NULL)
                    {
                        max_books = insert_borrower_book(max_books, leader_max_book->b_id, c_id, max_books->cost);
                    }
                    /* Now we need for every l_id to spot the max book */
                    for (int i = 0; i < N * N; i++)
                    {
                        BookBorrowers* traverse_max_list = max_books;
                        BookBorrowers* current_max_for_this_l_id = NULL;
                        while (traverse_max_list != NULL)
                        {
                            if (i == (traverse_max_list->b_id / N))
                            {
                                if (current_max_for_this_l_id == NULL)
                                {
                                    current_max_for_this_l_id = traverse_max_list;
                                }
                                else if (traverse_max_list->times_borrowed > current_max_for_this_l_id->times_borrowed
                                         || (traverse_max_list->times_borrowed == current_max_for_this_l_id->times_borrowed && traverse_max_list->cost > current_max_for_this_l_id->cost))
                                {
                                    current_max_for_this_l_id = traverse_max_list;
                                }
                            }
                            traverse_max_list = traverse_max_list->next;
                        }
                        /* If we found a valid book for this l_id print it */
                        if (current_max_for_this_l_id == NULL)
                        {
                            printf("No popular book for library %d\n", i);
                            fflush(stdout);
                        }
                        else
                        {
                            printf("Popular book %d for library %d\n", current_max_for_this_l_id->b_id, current_max_for_this_l_id->b_id / N);
                            fflush(stdout);
                        }
                    }
                    MessagePack message_done_2_coord = message_received;
                    message_done_2_coord.message_type = GET_MOST_POPULAR_BOOK_DONE;
                    MPI_Send(&message_done_2_coord, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
                    count_received = 0;
                }
            }
            else
            {
                /* We are an intermediate node just forward the message up */
                MessagePack popular_book_info = message_received;
                popular_book_info.message_type = GET_POPULAR_BK_INFO;
                MPI_Send(&popular_book_info, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
            }
        }
        /* CheckNumBooksLoaned event */
        else if (message_received.message_type == CHECK_NUM_BOOKS_LOAN)
        {
            /* Leader received CHECK_NUM_BOOKS_LOAN message perform a broadcast on the tree */
            int received_from = -1; // Track who sent the message first
            received_from = status_message.MPI_SOURCE;
            for (int i = 0; i < tree_node_details.num_neighbors; i++)
            {
                int neighbor_rank = c_id_2_rank(tree_node_details.neighbors[i]);

                /* Don't send back to parent or self */
                if (neighbor_rank != received_from && neighbor_rank != rank)
                {
                    MessagePack broadcast_msg = message_received;
                    broadcast_msg.message_type = CHECK_NUM_BOOKS_LOAN;
                    MPI_Send(&broadcast_msg, 1, MessagePackType, neighbor_rank, 0, MPI_COMM_WORLD);
                    // printf("Node %d forwarding to neighbor %d\n", rank, neighbor_rank);
                    // fflush(stdout);
                }
            }
            /* Now that we have broadcasted the message and we aren't the leader send our total borrows to leader */
            if (rank != tree_node_details.leader)
            {
                /* Find the most popular book */
                MessagePack inform_leader_books_borrowed = message_received;
                inform_leader_books_borrowed.message_type = NUM_BOOKS_LOANED;
                inform_leader_books_borrowed.c_id = c_id; /* Thats source */
                int total_borrows = 0;
                BookBorrowers* traversal = books;
                while (traversal != NULL)
                {
                    total_borrows = total_borrows + traversal->times_borrowed;
                    traversal = traversal->next;
                }
                inform_leader_books_borrowed.count_borrowed = total_borrows;
                MPI_Send(&inform_leader_books_borrowed, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
                MessagePack ack_2_receive_from_leader;
                MPI_Recv(&ack_2_receive_from_leader, 1, MessagePackType, tree_node_details.leader, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // printf("phra to ack APO rank %d\n", tree_node_details.leader);
                // fflush(stdout);
            }
        }
        else if (message_received.message_type == NUM_BOOKS_LOANED)
        {
            if (tree_node_details.leader == rank)
            {
                static int count_received = 0;
                static int count_borrows = 0;
                count_received++;
                count_borrows = count_borrows + message_received.count_borrowed;
                // printf("tosa mhnymata lambanei o leader gia plhrofories\n");
                // fflush(stdout);
                MessagePack ack_2_send_2_c_id = message_received;
                ack_2_send_2_c_id.message_type = ACK_NBL;
                MPI_Send(&ack_2_send_2_c_id, 1, MessagePackType, c_id_2_rank(message_received.c_id), 0, MPI_COMM_WORLD);
                if (count_received == floor((N * N * N) / 2) - 1)
                {
                    /* Include also the leader's count_borrows */
                    BookBorrowers* traversal = books;
                    while (traversal != NULL)
                    {
                        count_borrows = count_borrows + traversal->times_borrowed;
                        traversal = traversal->next;
                    }
                    printf("Loaner books %d\n", count_borrows);
                    /* Inform also the coordinator that we over */
                    MessagePack message_done_2_coord = message_received;
                    message_done_2_coord.message_type = CHECK_NUM_BOOKS_LOAN_DONE;
                    message_done_2_coord.count_borrowed = count_borrows;
                    MPI_Send(&message_done_2_coord, 1, MessagePackType, 0, 0, MPI_COMM_WORLD);
                    count_received = 0;
                    count_borrows = 0;
                }
            }
            else
            {
                /* Intermediate node just forward the message up */
                MessagePack num_books_borrowed_info = message_received;
                num_books_borrowed_info.message_type = NUM_BOOKS_LOANED;
                MPI_Send(&num_books_borrowed_info, 1, MessagePackType, c_id_2_rank(tree_node_details.parent), 0, MPI_COMM_WORLD);
            }
        }
        else if (message_received.message_type == TERMINATE)
        {
            /* Print neighbors for debug */
            // for (int i = 0; i < tree_node_details.num_neighbors; i++)
            // {
            //     printf("im c_id %d and my neighbors are %d and count_neighbors %d, also state received:%d and also my leader rank is%d and parent is %d\n", c_id, tree_node_details.neighbors[i],
            //            tree_node_details.num_neighbors, tree_node_details.received_neighbors[i], tree_node_details.leader, tree_node_details.parent);
            //     fflush(stdout);
            // }
            // print_borrower_books(books);
            online = 0;
        }
    }
}
/* This is the coordinator code */
int main(int argc, char** argv)
{
    int rank, size, N, count_borrowers, count_libraries, library_leader_l_id, borrower_leader_rank;
    MPI_Status status_message;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    N = atoi(argv[1]); /* N is the first parameter after the executable */
    count_borrowers = floor((N * N * N) / 2);
    count_libraries = N * N;

    create_mpi_types(&MessagePackType);

    /* Execute coordinator code if the rank is zero (read the testfile) */
    if (rank == 0)
    {
        // printf("COORDINATOR STARTED\n");
        // fflush(stdout);
        for (int i = 0; i < count_borrowers + count_libraries; i++)
        {
            while (1)
            {
                MessagePack waiting_to_receive_that_everyone_woke_up;
                MPI_Recv(&waiting_to_receive_that_everyone_woke_up, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (waiting_to_receive_that_everyone_woke_up.message_type == AWAKE)
                {
                    // printf("elava tosa awake %d\n", i);
                    // fflush(stdout);
                    break;
                }
            }
        }
        /* Open the file */
        char filename[100];
        sprintf(filename, "loaners_%d_libs_%d_np_%d.txt", count_borrowers, count_libraries, size);
        FILE* file = fopen(filename, "r");
        char line[50];
        int id1, id2;
        while (fscanf(file, "%s", line) != EOF)
        {
            /* Connect event */
            if (strcmp(line, "CONNECT") == 0)
            {
                fscanf(file, "%d %d", &id1, &id2);
                /* client1 receives connect from coordinator and considers client2 as its neighbor */
                /* int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) */
                MessagePack connect_message_initial;
                connect_message_initial.message_type = CONNECT;
                connect_message_initial.neighbor_to_become = id2;
                // printf("stelnw connect\n");
                // fflush(stdout);
                MPI_Send(&connect_message_initial, 1, MessagePackType, c_id_2_rank(id1), 0, MPI_COMM_WORLD);
                /* Now we wait for ACK and then we can move on */
                MessagePack ack_message_received;
                MPI_Recv(&ack_message_received, 1, MessagePackType, c_id_2_rank(id1), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else if (strcmp(line, "START_LE_LIBR") == 0)
            {
                MessagePack library_leader_message_initial;
                library_leader_message_initial.message_type = START_LEADER_ELECTION;
                /* Send START_LEADER_ELECTION message to every every library process (they have ranks [1, count_libraries]) */
                for (int i = 1; i <= count_libraries; i++)
                {
                    MPI_Send(&library_leader_message_initial, 1, MessagePackType, i, 0, MPI_COMM_WORLD);
                }
                /* Wait to receive LE_LIBR_DONE from the elected leader of the spanning tree */
                MessagePack leader_done_received;
                MPI_Recv(&leader_done_received, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                library_leader_l_id = leader_done_received.leader_id;
                // printf("L_ID OF THE ELECTED LEADER IS %d\n", leader_done_received.leader_id);
                // fflush(stdout);
            }
            else if (strcmp(line, "START_LE_LOANERS") == 0)
            {
                MessagePack borrower_leader_message_initial;
                borrower_leader_message_initial.message_type = START_LE_LOANERS;
                /* Send START_LE_LOANERS to every borrower (they have ranks [count_libraries+1, count_libraries+1+count_borrowers]) */
                for (int i = count_libraries + 1; i < count_libraries + 1 + count_borrowers; i++)
                {
                    MPI_Send(&borrower_leader_message_initial, 1, MessagePackType, i, 0, MPI_COMM_WORLD);
                }
                MessagePack leader_done_message_received;
                MPI_Recv(&leader_done_message_received, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status_message);
                borrower_leader_rank = leader_done_message_received.leader_rank;
                if (leader_done_message_received.message_type == LE_LOANERS_DONE)
                {
                    // printf("ola swsta phgan\n");
                    // fflush(stdout);
                }
                // printf("PAIDIA ELAVA LEADER APO BORROWER XAXAXAXA APISTEUTO KAI EINAI O KYRIOS %d\n", leader_done_message_received.leader_rank);
                // fflush(stdout);
            }
            else if (strcmp(line, "TAKE_BOOK") == 0)
            {
                int c_id, b_id;
                fscanf(file, "%d %d", &c_id, &b_id);
                /* Notify process with id c_id that I want to borrow book b_id */
                MessagePack want_to_borrow_book_message;
                want_to_borrow_book_message.message_type = WANT_BORROW_BOOK;
                want_to_borrow_book_message.c_id = c_id;
                want_to_borrow_book_message.b_id = b_id;
                MPI_Send(&want_to_borrow_book_message, 1, MessagePackType, c_id_2_rank(c_id), 0, MPI_COMM_WORLD);
                /* Check might need to remove */
                while (1)
                {
                    MessagePack waiting_to_end;
                    MPI_Recv(&waiting_to_end, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (waiting_to_end.message_type == DONE_FIND_BOOK)
                    {
                        // printf("vivliarakia hr8e coord\n");
                        // fflush(stdout);
                        break;
                    }
                }
            }
            else if (strcmp(line, "DONATE_BOOK") == 0)
            {
                int c_id, b_id, n_copies;
                fscanf(file, "%d %d %d", &c_id, &b_id, &n_copies);
                /* Notify borrower leader */
                MessagePack notify_borrower_c_id;
                notify_borrower_c_id.message_type = DONATE_BOOKS;
                notify_borrower_c_id.b_id = b_id;
                notify_borrower_c_id.n_copies = n_copies;
                notify_borrower_c_id.c_id = c_id;
                // printf("auta poy diabazw einai %d %d %d\n", c_id, b_id, n_copies);
                // fflush(stdout);
                /* Send that message to wake up the c_id borrower and for him to wake up his leader to start donations */
                MPI_Send(&notify_borrower_c_id, 1, MessagePackType, c_id_2_rank(c_id), 0, MPI_COMM_WORLD);
                /* Wait to receive donation done from borrower leader */
                while (1)
                {
                    MessagePack waiting_to_end;
                    MPI_Recv(&waiting_to_end, 1, MessagePackType, borrower_leader_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (waiting_to_end.message_type == DONATION_BOOKS_DONE)
                    {
                        break;
                    }
                }
                // printf("edw omws coord pame?\n");
                // fflush(stdout);
            }
            else if (strcmp(line, "GET_MOST_POPULAR_BOOK") == 0)
            {
                MessagePack initiate_max_borrowed_book_collection;
                initiate_max_borrowed_book_collection.message_type = GET_MOST_POPULAR_BOOK;
                MPI_Send(&initiate_max_borrowed_book_collection, 1, MessagePackType, borrower_leader_rank, 0, MPI_COMM_WORLD);
                /* Wait to receive GET_MOST_POPULAR_BOOK_DONE from borrower leader */
                while (1)
                {
                    MessagePack waiting_to_end;
                    MPI_Recv(&waiting_to_end, 1, MessagePackType, borrower_leader_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (waiting_to_end.message_type == GET_MOST_POPULAR_BOOK_DONE)
                    {
                        break;
                    }
                }
            }
            else if (strcmp(line, "CHECK_NUM_BOOKS_LOANED") == 0)
            {
                MessagePack initiate_check_num_books_libr_and_borr;
                initiate_check_num_books_libr_and_borr.message_type = CHECK_NUM_BOOKS_LOAN;
                MPI_Send(&initiate_check_num_books_libr_and_borr, 1, MessagePackType, borrower_leader_rank, 0, MPI_COMM_WORLD);
                MPI_Send(&initiate_check_num_books_libr_and_borr, 1, MessagePackType, l_id_2_rank(library_leader_l_id, N), 0, MPI_COMM_WORLD);
                int total_loans_library = 0, total_borrows_borrowers = 0;
                while (1)
                {
                    MessagePack waiting_to_end;
                    MPI_Status status;
                    MPI_Recv(&waiting_to_end, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                    if (waiting_to_end.message_type == CHECK_NUM_BOOKS_LOAN_DONE)
                    {
                        if (status.MPI_SOURCE == l_id_2_rank(library_leader_l_id, N))
                        {
                            total_loans_library = waiting_to_end.count_borrowed;
                        }
                        else
                        {
                            total_borrows_borrowers = waiting_to_end.count_borrowed;
                        }
                        break;
                    }
                }
                while (1)
                {
                    MessagePack waiting_to_end;
                    MPI_Status status;
                    MPI_Recv(&waiting_to_end, 1, MessagePackType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

                    if (waiting_to_end.message_type == CHECK_NUM_BOOKS_LOAN_DONE)
                    {
                        if (status.MPI_SOURCE == l_id_2_rank(library_leader_l_id, N))
                        {
                            total_loans_library = waiting_to_end.count_borrowed;
                        }
                        else
                        {
                            total_borrows_borrowers = waiting_to_end.count_borrowed;
                        }
                        break;
                    }
                }
                if (total_loans_library == total_borrows_borrowers)
                {
                    printf("CheckNumBooksLoaned SUCCESS\n");
                    fflush(stdout);
                }
                else
                {
                    printf("CheckNumBooksLoaned FAILED\n");
                    fflush(stdout);
                }
            }
        }
        /* Terminate libraries ranks [1, N^2] */
        for (int i = 1; i <= count_libraries; i++)
        {
            MessagePack term_message;
            term_message.message_type = TERMINATE;
            MPI_Send(&term_message, 1, MessagePackType, i, 0, MPI_COMM_WORLD);
        }
        /* Terminate borrower ranks [N^2 + 1, N^2+1+count_borrowers] */
        for (int i = count_libraries; i < count_libraries + count_borrowers; i++)
        {
            MessagePack term_message;
            term_message.message_type = TERMINATE;
            MPI_Send(&term_message, 1, MessagePackType, c_id_2_rank(i), 0, MPI_COMM_WORLD);
        }
    }
    else if (rank >= 1 && rank <= count_libraries)
    {
        library_handler(rank, N);
    }
    else
    {
        borrower_handler(rank, N);
    }
    MPI_Finalize();
    return 0;
}