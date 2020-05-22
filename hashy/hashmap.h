#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>
struct node{
    void* key;
    void* value;
    int empty;
    struct node *next;
};

struct hash_map{
    struct node* hashtable;
    int map_size;
    size_t (*hash)(void*);
    int (*cmp)(void*,void*);
    void (*key_destruct)(void*);
    void (*value_destruct)(void*);
    int current;
}; //Modify this!

struct hash_map* hash_map_new(size_t (*hash)(void*), int (*cmp)(void*,void*),
    void (*key_destruct)(void*), void (*value_destruct)(void*));
    
void add(struct hash_map* map1, struct node* map, void* k, void*v);

void resize(struct hash_map* map,int old_size);

void hash_map_put_entry_move(struct hash_map* map, void* k, void* v);

void hash_map_remove_entry(struct hash_map* map, void* k);

void* hash_map_get_value_ref(struct hash_map* map, void* k);

void hash_map_destroy(struct hash_map* map);

#endif
