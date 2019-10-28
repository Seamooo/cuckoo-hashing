#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define LOOPLIMIT 100

struct bucket{
	char *key;
	int value;
	bool used;
};

struct hashtable{
	struct bucket *buckets;
	int size;
	int population;
	char *salt;
	int salt_len;
};

struct cuckoo_table{
	struct hashtable table1;
	struct hashtable table2;
	int size; //number of buckets available
	int population;	//number of buckets populated
};

void set_salt(struct hashtable *table)
{
	for(int i = 0; i < table->salt_len; ++i){
		table->salt[i] = rand() % 126 + 1; //never can be 0
	}
	table->salt[table->salt_len] = '\0';
}

void init_hashtable(struct hashtable *table, int size){
	table->size = size;
	table->population = 0;
	table->buckets = calloc(table->size, sizeof(struct bucket));
	table->salt_len = 5;
	table->salt = (char*)malloc((table->salt_len + 1) * sizeof(char));
	set_salt(table);
}

void init_cuckoo_table(struct cuckoo_table *map)
{
	map->size = BUFSIZ;
	map->population = 0;
	init_hashtable(&(map->table1), map->size / 2);
	init_hashtable(&(map->table2), map->size / 2);
}

int fast_exponentiation(int base, int power, int modulo)
{
	int i = 1;
	int rv = 1;
	int curr = base;
	power >>= 1;
	while(power != 0){
		if(power & 1)
			rv = (rv * curr) % modulo;
		power >>= 1;
		curr = (curr * curr) % modulo;
	}
	return rv;
}

int hash(const char *key, const char *salt, int modulo)
{
	int rv = 0;
	int n = strlen(key) + strlen(salt);
	char *key_buffer = (char *)malloc((n + 1)*sizeof(char));
	strcpy(key_buffer, key);
	strcpy(key_buffer + strlen(key), salt);
	for(int i = 0; i < n; ++i)
		rv = (rv + key_buffer[i] * fast_exponentiation(31, n - i - 1, modulo)) % modulo;
	free(key_buffer);
	return rv;
}

//always called after re-salting or resizing
void rehash(struct cuckoo_table *map, int prev_buckets1_size, int prev_buckets2_size)
{
	//below corresponds to table 1 and table 2
	struct bucket *new_buckets1, *new_buckets2;
	new_buckets1 = calloc(map->table1.size, sizeof(struct bucket));
	new_buckets2 = calloc(map->table2.size, sizeof(struct bucket));
	while(true){
		bool premature_exit = false;
		for(int i = 0; i < prev_buckets1_size; ++i){
			char *curr_key = map->table1.buckets[i].key;
			int curr_val = map->table1.buckets[i].value;
			struct hashtable *table_to_insert = &(map->table1);
			struct bucket *buckets_to_insert = new_buckets1;
			bool inserted = false;
			for(int i = 0; i < LOOPLIMIT; ++i){
				int index = hash(curr_key, table_to_insert->salt, table_to_insert->size);
				if(!buckets_to_insert[index].used){
					buckets_to_insert[index].used = true;
					buckets_to_insert[index].key = curr_key;
					buckets_to_insert[index].value = curr_val;
					inserted = true;
					break;
				}
				char *temp_key = curr_key;
				int temp_val = curr_val;
				curr_key = buckets_to_insert[index].key;
				curr_val = buckets_to_insert[index].value;
				buckets_to_insert[index].key = temp_key;
				buckets_to_insert[index].value = temp_val;
				table_to_insert = (table_to_insert == &(map->table1)) ? &(map->table2) : &(map->table1);
				buckets_to_insert = (buckets_to_insert == new_buckets1) ? new_buckets2 : new_buckets1;
			}
			if(!inserted){
				set_salt(&(map->table1));
				set_salt(&(map->table2));
				memset(new_buckets1, 0, map->table1.size * sizeof(struct bucket));
				memset(new_buckets2, 0, map->table2.size * sizeof(struct bucket));
				premature_exit = true;
				break;
			}
		}
		if(!premature_exit)
			break;
	}
	free(map->table1.buckets);
	free(map->table2.buckets);
	map->table1.buckets = new_buckets1;
	map->table2.buckets = new_buckets2;
}

void increase_size(struct cuckoo_table *map)
{
	int prev_size, prev_table1_size, prev_table2_size;
	prev_table1_size = map->table1.size;
	prev_table2_size = map->table2.size;
	map->size *= 2;
	map->table1.size = map->size / 2;
	map->table2.size = map->size / 2;
	rehash(map, prev_table1_size, prev_table2_size);
}

void decrease_size(struct cuckoo_table *map)
{
	int prev_size, prev_table1_size, prev_table2_size;
	prev_table1_size = map->table1.size;
	prev_table2_size = map->table2.size;
	map->size /= 2;
	map->table1.size = map->size / 2;
	map->table2.size = map->size / 2;
	rehash(map, prev_table1_size, prev_table2_size);
}

void insert(struct cuckoo_table *map, char *key, int value)
{
	if(map->population + 1 > map->size * 0.7)
		increase_size(map);
	map->population++;
	bool inserted = false;
	struct hashtable *table_to_insert = &(map->table1);
	char *curr_key = key;
	int curr_val = value;
	while(!inserted){
		for(int i = 0; i < LOOPLIMIT; ++i){
			int index = hash(curr_key, table_to_insert->salt, table_to_insert->size);
			if(!table_to_insert->buckets[index].used){
				table_to_insert->buckets[index].used = true;
				table_to_insert->buckets[index].key = curr_key;
				table_to_insert->buckets[index].value = curr_val;
				inserted = true;
				break;
			}
			char *temp_key = curr_key;
			int temp_val = curr_val;
			curr_key = table_to_insert->buckets[index].key;
			curr_val = table_to_insert->buckets[index].value;
			table_to_insert->buckets[index].key = temp_key;
			table_to_insert->buckets[index].value = temp_val;
			table_to_insert = (table_to_insert == &(map->table1)) ? &(map->table2) : &(map->table1);
		}
		if(!inserted){
			set_salt(&(map->table1));
			set_salt(&(map->table2));
			rehash(map, map->table1.size, map->table2.size);
		}
	}
}

void print_map(struct cuckoo_table *map)
{
	printf("{");
	bool first = true;
	for(int i = 0; i < map->table1.size; ++i){
		if(map->table1.buckets[i].used){
			if(!first)
				printf(", ");
			else
				first = false;
			printf("\"%s\":%d", map->table1.buckets[i].key, map->table1.buckets[i].value);

		}
	}
	for(int i = 0; i < map->table2.size; ++i){
		if(!first)
				printf(", ");
			else
				first = false;
		if(map->table2.buckets[i].used)
			printf("\"%s\":%d", map->table2.buckets[i].key, map->table2.buckets[i].value);
	}
	printf("}\n");
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	//hashtable implemented hashes string keys to integer values
	//close to map<string,int>
	struct cuckoo_table hashmap;
	init_cuckoo_table(&hashmap);
	//remember to figure out how to free memory properly
	print_map(&hashmap);
	exit(EXIT_SUCCESS);
}
