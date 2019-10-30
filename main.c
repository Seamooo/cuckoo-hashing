#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>

#define LOOPLIMIT 100
#define BLOCKSIZE 64 //size in bytes
#if (BLOCKSIZE % 64 != 0)
#error BLOCKSIZE must be a multiple of 64 bytes
#endif
#define RET_FAILURE EXIT_FAILURE
#define RET_SUCCESS EXIT_SUCCESS
//debug macros
#define p() printf("here\n"); fflush(stdout);
#define pint(x) printf("%s = %d\n", #x, x); fflush(stdout);
#define puint(x) printf("%s = %u\n", #x, x); fflush(stdout);
#define pstr(x) printf("%s = %s\n", #x, x); fflush(stdout);
#define phex(x, nbytes) printf("%s = ", #x); for(int _i = 0; _i < nbytes; ++_i){printf("%02hhx", ((char *)x)[_i]);} printf("\n");fflush(stdout);
#define pbar() printf("--------------------------------------------------------------------------------\n");fflush(stdout);

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
	table->salt_len = 6;
	table->salt = (char*)malloc((table->salt_len + 1) * sizeof(char));
	set_salt(table);
}

void init_cuckoo_table(struct cuckoo_table *map)
{
	map->size = BUFSIZ;
	map->population = 0;
	init_hashtable(&(map->table1), map->size);
	init_hashtable(&(map->table2), map->size);
}

void del_cuckoo_table(struct cuckoo_table *map)
{
	for(int i = 0; i < map->table1.size; ++i)
		if(map->table1.buckets[i].used)
			free(map->table1.buckets[i].key);
	free(map->table1.buckets);
	for(int i = 0; i < map->table2.size; ++i)
		if(map->table2.buckets[i].used)
			free(map->table2.buckets[i].key);
	free(map->table2.buckets);
}

int hash_vals[] = {
	0x92DB44A2, 0x545A89FC, 0x1507F26C, 0x319D9FEF, 0xD65173C0, 0x974B16E7,
	0xAA1EEA9E, 0x2DC514FF, 0x0FEF07D2, 0x9006B98D, 0x2467EE06, 0xE8E94BB8,
	0x8D73BEDD, 0xDFC44304, 0x2A181C6A, 0x7B0DA832, 0x41F9F5EB, 0x9386CF16,
	0x90E17BD3, 0xF7B0285B, 0x75C52CEB, 0x360AE8E1, 0x70B670E0, 0x895729CB,
	0xD014702A, 0xAD396888, 0x4FEEFFB9, 0xD77D19FF, 0x34394FC2, 0x2F609688,
	0x1BEBAB82, 0x8A1D6C72, 0x9B44C527, 0xDFEF0A45, 0xD308BAB8, 0xFEBE14D1,
	0xF485E29C, 0xB1EF6478, 0xCB8FAF32, 0x06271ACC, 0x76BB4DB6, 0x02C504F6,
	0xEE011A43, 0x047C866E, 0xF806906E, 0x9F33DF86, 0x2337DA62, 0xB081B5C2,
	0x239B9B76, 0xD2A42B53, 0x01543C39, 0x69262E72, 0x8A0A5B74, 0xDEC05B06,
	0x09871CF1, 0x1BC24CBC, 0xD608FDC2, 0xA883115C, 0xE8071104, 0x4586905D,
	0x96D5971F, 0xF6082404, 0xE0CD8BA8, 0x192CB1BA, 0x7B1F1C90, 0x0D63AA5F,
	0xD8204CC7, 0x8F2BABA0, 0xD7A1AF13, 0xE77ADC64, 0x597B8854, 0xC7171A07,
	0xC6919253, 0x6F2C884B, 0xC7EAE690, 0xBBB3552E, 0xB4A265CF, 0xA5752970,
	0x4A6D4DA9, 0x425B82F9, 0x139ACE8B, 0x01E37BBC, 0xC4911097, 0x18004918,
	0x80D1B5B8, 0x24A81AA7, 0x3A97D26D, 0x3C3CC5D0, 0xE99CD549, 0xBBE0D2BF,
	0xC0DE2E2F, 0x725B9522, 0x4A04B36B, 0xBB36F5C5, 0x9A3B17BC, 0x50273216,
	0x5D58099C, 0xBF99B61F, 0x81B31B80, 0x565D47D4, 0xBC70A098, 0xE442A644,
	0x87882286, 0xC64D4017, 0x61A74CE9, 0xC69D2106, 0xE81336BA, 0xD027BE90,
	0x6316D27E, 0x78DCD1E4, 0xE65DA480, 0x08ED6094, 0xCDDF444C, 0x56C2D240,
	0x1FD46BE7, 0xE726FF08, 0x41E4170D, 0x8F1E2F24, 0xB5C960C6, 0x52A5C304,
	0x23BF4FDF, 0x656118EF, 0x9148EFF8, 0xC33AF113, 0x2FCE7413, 0x4E2B0EE6,
	0x7F84CB2E, 0x1C08C045, 0x994BBD9A, 0x1EB9C455, 0x4B9314DD, 0x5C51A921,
	0xB7782CFF, 0xF3962AF4, 0x9306F674, 0x64ADF343, 0x6419F5AB, 0x9374CD44,
	0x6F5DECA7, 0x29773163, 0xD36D9FC4, 0x9DB8EB45, 0xFA0AE184, 0x79384C99,
	0xF2D3B402, 0xB07BACDE, 0x1F75BA2B, 0xC2499F94, 0x9D20AB81, 0xEDD36FF4,
	0x9E457AD5, 0x18412F9A, 0xB13DA768, 0xF3D2A9BE, 0x136E3EBD, 0xED08373A,
	0x95450C54, 0x38259874, 0x723D4F96, 0x5274BE6E, 0x9BC72E92, 0xECD04E38,
	0x6ADD9E2E, 0xA5CCDFAA, 0x6DF0B57C, 0x489FB11D, 0x7E50F43D, 0xC8A56713,
	0xC62A8817, 0x0E77C72E, 0xD9D162DC, 0xC2E63ACD, 0xE3AC4FED, 0xC6248E84,
	0x1E8B7D4A, 0x8328B351, 0x83E591A6, 0x06121127, 0x9C3BCFBB, 0x847B7409,
	0xBB92D8CC, 0x16DAEE33, 0x30E1C833, 0xA21C1462, 0x5BBC857A, 0x560C3B6F,
	0xF210F5D3, 0x1E439682, 0xFE92A388, 0x33FF92A4, 0x0619062C, 0x3F775B08,
	0xE7632D66, 0x9D57CB2F, 0x8F65D167, 0x7DAC4EFE, 0x1F2CB63C, 0x84566044,
	0x41D11303, 0x64C8B547, 0xDC2FAB74, 0xCCF1C1B9, 0x927C1493, 0x9BE38991,
	0x0E073221, 0xCB1DDB67, 0x541478C6, 0xFFD57AF9, 0xB9D20978, 0x79DFDD41,
	0x5AF75F72, 0xE4E665A6, 0xE17174C2, 0x2C19ACCC, 0x9072D25C, 0x4E437362,
	0xC38FA785, 0xDA3249C5, 0xBA15BD29, 0x3143BF08, 0xEF61E2D9, 0x9AD6DE58,
	0x8ECE6391, 0x3BF8648A, 0x5E51915B, 0xEAD6CCC8, 0xE7A2E927, 0x0274FE67,
	0x891DE355, 0xDC628663, 0xC99FD88F, 0x63BEC848, 0x67FCA941, 0x2A186F74,
	0x9EA6B2E9, 0x49E909C7, 0xE9F3ECC8, 0x73C6002D, 0x25F4E413, 0x2113AEF5,
	0xDB8A2B9A, 0x6ABC6942, 0x5F1AE07C, 0x7184586D, 0x39D548EE, 0x9F427C5B,
	0x35A8299B, 0x474A577F, 0xF6DA0B20, 0xC8B4D686, 0xB3432DF9, 0xD24C27EC,
	0xC9E6D86C, 0x024A577B, 0x1279004E, 0x5105826A,
};

int hash(const char *key, const char *salt, int modulo)
{
	int rv = 0;
	uint32_t hash[] = {0x4B01EFCE, 0x9CC35312, 0xBB7FE3D8, 0x38AD72BE};
	int len = strlen(key) + strlen(salt);
	int nblocks = (int)ceil(len / (double)BLOCKSIZE);
	unsigned char *key_buffer = (unsigned char *)malloc(nblocks*BLOCKSIZE*sizeof(unsigned char));
	memset(key_buffer, 0xFF, nblocks*BLOCKSIZE*sizeof(char));
	memcpy(key_buffer, key, strlen(key));
	memcpy(key_buffer + strlen(key), salt, strlen(salt));
	//untested hashing algorithm based off md5 sums
	for(int i = 0; i < nblocks; ++i){
		uint32_t temp_hash[4];
		memcpy(temp_hash, hash, 4*sizeof(uint32_t));
		int temp = 0;
		int index = 0;
		for(int j = 0; j < BLOCKSIZE / 4; ++j){
			uint32_t currint = (((unsigned char)hash_vals[key_buffer[i*BLOCKSIZE + j*4]]) + ((unsigned char)hash_vals[key_buffer[i*BLOCKSIZE + j*4 + 1]]));
			currint ^= hash_vals[(unsigned char)key_buffer[i*BLOCKSIZE + j*4 + 2]];
			currint += hash_vals[(unsigned char)key_buffer[i*BLOCKSIZE + j*4 + 3]];
			//below switch statement needs to be changed if BLOCKSIZE > 64
			switch(j / (BLOCKSIZE / 16)){
			case 0:
				temp = (temp_hash[1] & temp_hash[2]) | (~temp_hash[1] & temp_hash[3]);
				index = j;
				break;
			case 1:
				temp = (temp_hash[3] & temp_hash[1]) | (~temp_hash[3] & temp_hash[2]);
				index = (j * 5 + 1) % (BLOCKSIZE / 4);
				break;
			case 2:
				temp = temp_hash[1] ^ temp_hash[2] ^ temp_hash[3];
				index = (j * 3 + 5) % (BLOCKSIZE / 4);
				break;
			case 3:
				temp = temp_hash[2] ^ (temp_hash[1] | (~temp_hash[3]));
				index = (j*7) % (BLOCKSIZE / 4);
				break;
			}
			temp = temp + temp_hash[0] + hash_vals[j] + hash_vals[key_buffer[i*BLOCKSIZE + index]];
			temp_hash[0] = temp_hash[3];
			temp_hash[3] = temp_hash[2];
			temp_hash[2] = temp_hash[1];
			temp_hash[1] += (temp << (currint%32)) | (temp >> (32 - (currint%32)));
		}
		hash[0] += temp_hash[0];
		hash[1] += temp_hash[1];
		hash[2] += temp_hash[2];
		hash[3] += temp_hash[3];
	}
	free(key_buffer);
	return (hash[0] + hash[1] + hash[2] + hash[3]) % modulo;
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
			if(!map->table1.buckets[i].used)
				continue;
			char *curr_key = map->table1.buckets[i].key;
			int curr_val = map->table1.buckets[i].value;
			struct hashtable *table_to_insert = &(map->table1);
			struct bucket *buckets_to_insert = new_buckets1;
			bool inserted = false;
			for(int j = 0; j < LOOPLIMIT; ++j){
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
		if(premature_exit)
			continue;
		for(int i = 0; i < prev_buckets2_size; ++i){
			if(!map->table2.buckets[i].used)
				continue;
			char *curr_key = map->table2.buckets[i].key;
			int curr_val = map->table2.buckets[i].value;
			struct hashtable *table_to_insert = &(map->table2);
			struct bucket *buckets_to_insert = new_buckets2;
			bool inserted = false;
			for(int j = 0; j < LOOPLIMIT; ++j){
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
		if(!premature_exit){
			break;
		}
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
	map->table1.size = map->size;
	map->table2.size = map->size;
	rehash(map, prev_table1_size, prev_table2_size);
}

void decrease_size(struct cuckoo_table *map)
{
	int prev_size, prev_table1_size, prev_table2_size;
	prev_table1_size = map->table1.size;
	prev_table2_size = map->table2.size;
	map->size /= 2;
	map->table1.size = map->size;
	map->table2.size = map->size;
	rehash(map, prev_table1_size, prev_table2_size);
}

int lookup(struct cuckoo_table *map, const char *key, int *value_rv)
{
	int index = hash(key, map->table1.salt, map->table1.size);
	if(map->table1.buckets[index].used){
		if(strcmp(key, map->table1.buckets[index].key) == 0){
			if(value_rv != NULL)
				*value_rv = map->table1.buckets[index].value;
			return RET_SUCCESS;
		}
	}
	index = hash(key, map->table2.salt, map->table2.size);
	if(map->table2.buckets[index].used){
		if(strcmp(key, map->table2.buckets[index].key) == 0){
			if(value_rv != NULL)
				*value_rv = map->table2.buckets[index].value;
			return RET_SUCCESS;
		}
	}
	return RET_FAILURE;
}

int insert(struct cuckoo_table *map, char *key, int value)
{
	//exit if key already exists
	if(lookup(map, key, NULL) == RET_SUCCESS)
		return RET_FAILURE;
	if(map->population + 1 > map->size * 0.7)
		increase_size(map);
	map->population++;
	bool inserted = false;
	struct hashtable *table_to_insert = &(map->table1);
	char *curr_key = strdup(key);
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
	return RET_SUCCESS;
}

int delete(struct cuckoo_table *map, char *key)
{
	int index = hash(key, map->table1.salt, map->table1.size);
	if(map->table1.buckets[index].used){
		if(strcmp(key, map->table1.buckets[index].key) == 0){
			free(map->table1.buckets[index].key);
			map->table1.buckets[index].used = false;
			map->population--;
			if(map->population < 0.3 * map->size && map->size > BUFSIZ)
				decrease_size(map);
			return RET_SUCCESS;
		}
	}
	index = hash(key, map->table2.salt, map->table2.size);
	if(map->table2.buckets[index].used){
		if(strcmp(key, map->table2.buckets[index].key) == 0){
			free(map->table2.buckets[index].key);
			map->table2.buckets[index].used = false;
			map->population--;
			if(map->population < 0.3 * map->size && map->size > BUFSIZ)
				decrease_size(map);
			return RET_SUCCESS;
		}
	}
	return RET_FAILURE;
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
		if(map->table2.buckets[i].used){
			if(!first)
				printf(", ");
			else
				first = false;
			printf("\"%s\":%d", map->table2.buckets[i].key, map->table2.buckets[i].value);
		}
	}
	printf("}\n");
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	//hashtable implemented hashes string keys to integer values
	//close to map<string,int>
	struct cuckoo_table map;
	init_cuckoo_table(&map);
	const int total_keys = 2000;
	char *keys[total_keys];
	for(int i = 0; i < total_keys; ++i){
		keys[i] = malloc(20*sizeof(char));
		sprintf(keys[i], "%d-key", i);
		if(insert(&map, keys[i], i) == RET_FAILURE)
			fprintf(stderr, "could not insert key %s\n", keys[i]);
	}
	int ret_val;
	for(int i = 0; i < 20; ++i){
		int key_index = rand() % total_keys;
		if(rand() % 5 == 0){
			if(delete(&map, keys[key_index]) == RET_FAILURE)
				fprintf(stderr, "couldn't delete key: %s\n", keys[key_index]);
			else
				printf("deleted key: %s\n", keys[key_index]);
		}
		if(lookup(&map, keys[key_index], &ret_val) == RET_FAILURE)
			fprintf(stderr, "couldn't find key: %s\n", keys[key_index]);
		else
			printf("found {\"%s\":%d}\n", keys[key_index], ret_val);
	}
	if(lookup(&map, "no-key", &ret_val) == RET_FAILURE)
		fprintf(stderr, "couldn't find key: no-key\n");
	for(int i = 0; i < total_keys; ++i)
		free(keys[i]);
	del_cuckoo_table(&map);
	exit(EXIT_SUCCESS);
}
