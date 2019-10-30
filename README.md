Cuckoo Hashing
==============
An implementation of a hashmap in c using cuckoo hashing and a hashing algorithm based off md5 sums

Upsizes map when population is larger than 70% of the capacity of the hashtable  
Downsizes when population is smaller than 30% of the capacity of the hashtable

Usage
----
Create cuckoo_table struct  
Call init_cuckoo_table and pass the address of this struct  
Use insert, delete and lookup for adding key value pairs, searching for key value pairs and deleting key value pairs  
Call del_cuckoo_table to cleanup memory

Functions
-------

`void init_cuckoo_table(struct cuckoo_table *map)`  
Initialises cuckoo table and allocates memory  
Parameters:  
* map: address of cuckoo_table struct to initialise  

Return value:  
* void  

`void del_cuckoo_table(struct cuckoo_table *map)`  
Frees memory of cuckoo table struct allocated by init  
Parameters:  
* map: address of cuckoo_table to destruct  

Return value:  
* void  

`int insert(struct cuckoo_table *map, char *key, int value)`  
Inserts key value pair into map  
Parameters:  
* map: address of cuckoo_table struct to perform insert  
* key: key to insert  
* value: value to insert  

Return value:  
* RET_SUCCESS on successful insertion  
* RET_FAILURE if key already exists  

`int lookup(struct cuckoo_table *map, const char *key, int *value_rv)`  
performs lookup on key in map, setting value_rv to be the corresponding value if a corresponding key exists and value_rv != NULL  
Parameters:  
* map: address of cuckoo_table struct to perform lookup within  
* key: key to lookup  
* value_rv: address of integer to store result  

Return value:  
* RET_SUCCESS on successful return  
* RET_FAILURE if key is not found  

`int delete(struct cuckoo_table *map, char *key)`  
Deletes key from map if such a key exists  
Parameters:  
* map: address of cuckoo_table struct to perform deletion  
* key: key to remove  

Return value:  
* RET_SUCCESS on successful deletion  
* RET_FAILURE if key did not exist  
