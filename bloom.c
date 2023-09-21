// This file handles the bloom filter used to store and compare words within the sets

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


uint64_t count;   // Total number of members in bloom filter
int *bit_array; // Bitarray used to store values
int *seeds;     // Seeds used for hash function
int hash_count; // Number of hash functions
uint64_t arr_size;  // Size of the bit array

// Function headers

void init_filter(size_t  members, float fp_rate);
int init_array(int ceil_pow); 
void set_bit(size_t position);
int get_bit(size_t position);
uint64_t hash(char *value, int seed);
void gen_primes();

void insert(char *value);
bool test(char *value);

// Initializes the bloom filter to contain size_t members and have a false positive rate of fp_rate (usually 0.01)
void
init_filter(uint64_t members, float fp_rate)
{
        count = members;
        // Calculating the required size
        float r = count * log(fp_rate) / log(0.618);
        // Find corresponding number of filters k
        hash_count = (r / count * log(2));
        // Find a power of 2 greater than r
        int ceil_pow = ceil(log(r) / log(2));
        // Create an array of bits
        init_array(ceil_pow);
        arr_size = pow(2, ceil_pow);
        // Create hash_count many hash functions
        seeds = malloc(hash_count * sizeof(int));
        gen_primes();
}

// Generates the necessary primes for hash count
void
gen_primes()
{
        // First generate a larger list of primes
        unsigned short primes[3400];
        int counter = 1;
        bool prime;
        for (int i = 0; i < 3400;) {
                counter ++;
                prime = true;
                for (int j = 0; j < i; j++) {
                        if (counter % primes[i] == 0) {
                                prime = false;
                                break;
                        }
                }
                if (prime) {
                        primes[i] = counter;
                        i++;
                }
        }

        // Generate the random primes
        int index;
        for (int i = 0; i < hash_count; i++) {
                index = rand() % 1400 + 2000;
                seeds[i] = index;
        }
}

// Initializes bit array to be an array of 2^ceil_pow bits
int
init_array(int ceil_pow) {
        // Size of array in bytes
        size_t size;
        if (ceil_pow < 2) {
                size = 1;
        } 
        else size = pow(2, ceil_pow - 3);
        // Create array to be entirely 0
        bit_array = calloc(size, sizeof(int));
        if (bit_array == NULL) {
                fprintf(stderr, "Calloc failed");
                return 1;
        }
        return 0;
}


// Insert given value into the bloom filter
void
insert(char *value) {
        // Go through each hash function
        for (int i = 0; i < hash_count; i++) {
                set_bit(hash(strlwr(value), seeds[i]));
        }
}
// Tests if a given value is in the bloom filter
bool
test(char *value) {
        // Go through each hash function
        bool included = true;
        for (int i = 0; i < hash_count; i++) {
                if (!get_bit(hash(strlwr(value), seeds[i]))) {
                        included = false;
                }
        }
        return included;
}

// Hashes the given value to the bit array, with a given seed, uses djb
uint64_t
hash(char *value, int seed) {
        unsigned long hash = seed;
        int c;

        while ((c = *value++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash % arr_size;
}


// Getter and setter methods for the bit array
void
set_bit(uint64_t position) {
        bit_array[position / 8] |= (1 << (position % 8));
}

int
get_bit(uint64_t position) {
        return (bit_array[position/8] >> (position % 8)) & 0x1;
}

void
check() {
        for (int i =0; i < 40; i++) {
                if (bit_array[i] != 0)
                printf("%x\n", bit_array[i]);
        }
}