#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#define TABLE_SIZE 3571 


//data structure to lookup user names
typedef struct lookup_table_entry{
    char *user_name;
    int sock_fd;
    struct lookup_table_entry *next;
}lookup_table_entry;

typedef lookup_table_entry** lookup_table;

//hash function
int djb2(const char *key);

//print table content to buf
void table_dump(lookup_table table, char *buf);

//initialize empty lookup table
lookup_table table_init(void);

//lookup for an user in the table
lookup_table_entry *table_lookup(lookup_table table, const char *key);

//lookup for a socket id in the table
lookup_table_entry *table_query_id(lookup_table table, int sockfd);

//insert an user to the table
lookup_table_entry * table_insert(lookup_table table, const char *user_name, int sock_fd);

//delete an user from the table
int table_evict(lookup_table table, const char *key);

//delete the table
void table_destroy(lookup_table table);

#endif //lookup_table.h