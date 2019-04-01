#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lookup_table.h"

int djb2(const char *key){
	int hash = 2*strlen(key)+1;
	int char_int;
	int char_count;
	
	for (char_count=0; char_count<strlen(key); char_count++)
	{
        char_int = (int) key[char_count];
        hash = hash * 33 + char_int;
	}
    return abs(hash % (TABLE_SIZE));
}

void table_dump(lookup_table table, char *buf){
	int cursor = 0;
	for (int i = 0; i < TABLE_SIZE; ++i){
		if (table[i] != NULL){
			lookup_table_entry *cur = table[i];
			while(cur != NULL){
				sprintf(buf + cursor,"%s\n",cur->user_name);
				cursor += strlen(cur->user_name) + 1;
				// printf("%s\n", cur->user_name);
				cur = cur->next;
			}
		}
	}
}

lookup_table table_init(void){
    lookup_table table;
	assert(table = (lookup_table )malloc(sizeof(lookup_table_entry *) * TABLE_SIZE));
	memset(table, 0, sizeof(lookup_table_entry *) * TABLE_SIZE);
	return table;
}

lookup_table_entry *table_lookup(lookup_table table, const char *key){
	int bin = djb2(key);
	for (lookup_table_entry* cur = table[bin];cur != NULL;cur = cur->next)
		if (strcmp(cur->user_name,key)==0) return cur;

	return NULL;
}

lookup_table_entry *table_query_id(lookup_table table, int sockfd){
	for (int i = 0; i < TABLE_SIZE; ++i){
		if (table[i] != NULL){
			lookup_table_entry *cur = table[i];
			while(cur != NULL){
				if (cur->sock_fd == sockfd) return cur;
				cur = cur->next;
			}
		}
	}
	return NULL;
}

lookup_table_entry * table_insert(lookup_table table, const char *user_name, int sock_fd){
	lookup_table_entry *search_entry = table_lookup(table,user_name);
	if (search_entry != NULL) return NULL;
	else{
        int bin = djb2(user_name);
        lookup_table_entry *new_entry = (lookup_table_entry *)malloc(sizeof(lookup_table_entry));
        assert(new_entry);
        new_entry->user_name = strdup(user_name);
        new_entry->sock_fd = sock_fd;
        new_entry->next = table[bin];
        table[bin] = new_entry;
        return new_entry;
	}
}

int table_evict(lookup_table table, const char *key){
    int bin = djb2(key);
    lookup_table_entry *prev = NULL, *cur = table[bin];
    for (cur = table[bin]; cur != NULL; cur = cur->next){
        if (strcmp(cur->user_name,key)==0){
            if (prev == NULL) table[bin] = cur->next;
            else prev->next = cur->next;
            free(cur->user_name);
            free(cur);
            return 1;
        }
        prev = cur;
    }
    return 0;
}

void table_destroy(lookup_table table){
	for(int i = 0; i < TABLE_SIZE; ++i){
		lookup_table_entry *cur = table[i];
		lookup_table_entry *next = NULL;
		while (cur != NULL){
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	free(table);
}