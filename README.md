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
