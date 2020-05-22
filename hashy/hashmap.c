#include <stdlib.h>
#include "hashmap.h"
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t stack_mutex = PTHREAD_MUTEX_INITIALIZER;
struct hash_map* hash_map_new(size_t (*hash)(void*), int (*cmp)(void*,void*),
	void (*key_destruct)(void*), void (*value_destruct)(void*)) {
	if(hash==NULL||cmp==NULL||key_destruct==NULL||value_destruct==NULL){
		return NULL;
	}
	struct hash_map* table=malloc(sizeof(struct hash_map));
	table->map_size=32;
	table->hashtable=malloc(sizeof(struct node)*table->map_size);
	for(int i=0;i<table->map_size;i++){
		table->hashtable[i].empty=0;
		table->hashtable[i].key=NULL;
		table->hashtable[i].value=NULL;
		table->hashtable[i].next=NULL;
	} 
	table->current=0;
	table->hash=hash;
	table->cmp=cmp;
	table->key_destruct=key_destruct;
	table->value_destruct=value_destruct;
	return table;
}

void add(struct hash_map* map1, struct node* map, void* k, void*v){
	size_t pos=map1->hash(k);
	pos=pos%map1->map_size;
	if(map[pos].empty==0){
		map[pos].key=k;
		map[pos].value=v;
		map[pos].empty=1;
		map1->current+=1;
		if(map1->current*10>=6*map1->map_size){
			resize(map1,map1->map_size);
		}
	}
	else{
		if(map1->cmp(k,map[pos].key)==1){
				map1->key_destruct(map[pos].key);
				map1->value_destruct(map[pos].value);
				map[pos].value=v;
				map[pos].key=k;
			}
		else{
		struct node* cursor=&map[pos];
		while(cursor->next!=NULL){
			if(map1->cmp(k,cursor->key)==1){
				break;
			}
			cursor=cursor->next;
		}
		if(cursor->next!=NULL){
			map1->key_destruct(cursor->key);
			map1->value_destruct(cursor->value);
			cursor->value=v;
			cursor->key=k;
		}
		else{
			struct node* newnode=malloc(sizeof(struct node));
			newnode->key=k;
			newnode->value=v;
			newnode->empty=1;
			newnode->next=NULL;
			cursor->next=newnode;
		}
	}
	}
}

void resize(struct hash_map* map,int old_size){
	map->map_size=1.5*map->map_size;
	//printf("called %d,%d\n",old_size,map->map_size);
	struct node* new_map=malloc(sizeof(struct node)*map->map_size);
	for(int i=0;i<map->map_size;i++){
		new_map[i].empty=0;
		new_map[i].key=NULL;
		new_map[i].value=NULL;
		new_map[i].next=NULL;
	}
	map->current=0;
	for(int i=0;i<old_size;i++){
		if(map->hashtable[i].empty==1){
			add(map,new_map,map->hashtable[i].key,map->hashtable[i].value);
		if(map->hashtable[i].next!=NULL){
				struct node* cursor=map->hashtable[i].next;
				struct node* tmp=NULL;
				while(cursor!=NULL){
					tmp=cursor->next;
					add(map,new_map,cursor->key,cursor->value);
					cursor=tmp;
				}
			}
		}
	}
	for(int i=0;i<old_size;i++){
		if(map->hashtable[i].empty==1){
		if(map->hashtable[i].next!=NULL){
				struct node* cursor=map->hashtable[i].next;
				struct node* tmp=NULL;
				while(cursor!=NULL){
					tmp=cursor->next;
					free(cursor);
					cursor=tmp;
				}
			}
}
	}
	free(map->hashtable);
	map->hashtable=new_map;
}

void hash_map_put_entry_move(struct hash_map* map, void* k, void* v) {
	size_t pos=map->hash(k);
	//printf("put is %zu\n",pos);
	pos=pos%map->map_size;
	pthread_mutex_lock(&stack_mutex);
	add(map,map->hashtable,k,v);
	pthread_mutex_unlock(&stack_mutex);
}

void hash_map_remove_entry(struct hash_map* map, void* k) {
	pthread_mutex_lock(&stack_mutex);
	//printf("remove is %zu\n",map->hash(k));
	for(int i=0;i<map->map_size;i++){
		if(map->hashtable[i].empty==1){
		if(map->cmp(k,map->hashtable[i].key)==1){
				map->hashtable[i].empty=0;
				map->key_destruct(map->hashtable[i].key);
				map->hashtable[i].key=NULL;
				map->value_destruct(map->hashtable[i].value);
				map->hashtable[i].value=NULL;
				if(map->hashtable[i].next!=NULL){
					map->hashtable[i].empty=1;
					map->hashtable[i].key=map->hashtable[i].next->key;
					map->hashtable[i].value=map->hashtable[i].next->value;
					struct node*tmp=map->hashtable[i].next;
					map->hashtable[i].next=map->hashtable[i].next->next;
					free(tmp);
				}
				break;
				}
		if(map->hashtable[i].next!=NULL){
				struct node* cursor=&map->hashtable[i];
				struct node* before=NULL;
				while(cursor!=NULL){
					if(map->cmp(k,cursor->key)==1){
						before->next=cursor->next;
						map->key_destruct(cursor->key);
						cursor->key=NULL;
						map->value_destruct(cursor->value);
						cursor->value=NULL;
						free(cursor);
						break;
					}
					before=cursor;
					cursor=cursor->next;
			}
		}
		}
	}
	pthread_mutex_unlock(&stack_mutex);
}

void* hash_map_get_value_ref(struct hash_map* map, void* k) {
	pthread_mutex_lock(&stack_mutex);
	//printf("get is %zu,%d,%d\n",map->hash(k)%map->map_size,map->current,map->map_size);
	for(int i=0;i<map->map_size;i++){
		if(map->hashtable[i].empty==1){
		if(map->cmp(k,map->hashtable[i].key)==1){
			pthread_mutex_unlock(&stack_mutex);
				return map->hashtable[i].value;
			}
		if(map->hashtable[i].next!=NULL){
				struct node* cursor=&map->hashtable[i];
				while(cursor!=NULL){
					if(map->cmp(k,cursor->key)==1){
						pthread_mutex_unlock(&stack_mutex);
						return cursor->value;
					}
					cursor=cursor->next;
				}
			}
		}
}
pthread_mutex_unlock(&stack_mutex);
	return NULL;
}

void hash_map_destroy(struct hash_map* map) {
	pthread_mutex_lock(&stack_mutex);
	for(int i=0;i<map->map_size;i++){
		if(map->hashtable[i].key!=NULL){
		map->key_destruct(map->hashtable[i].key);
		map->value_destruct(map->hashtable[i].value);
		if(map->hashtable[i].next!=NULL){
				struct node* cursor=map->hashtable[i].next;
				struct node* tmp=NULL;
				while(cursor!=NULL){
					tmp=cursor->next;
					map->key_destruct(cursor->key);
					map->value_destruct(cursor->value);
					free(cursor);
					cursor=tmp;
				}
			}
}
	}
pthread_mutex_unlock(&stack_mutex);
free(map->hashtable);
free(map);
}