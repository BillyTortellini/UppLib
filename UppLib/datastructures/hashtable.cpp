#include "hashtable.hpp"
int valid_primes[] = {
    11,
    23,
    47,
    97,
    197,
    397,
    797,
    1597,
    3203,
    6421,
    12853,
    25717,
    51437,
    102877,
    205759,
    411527,
    823117,
    1646237,
};

int hashtable_find_next_suitable_prime_size(int capacity)
{
    for (int i = 0; i < sizeof(valid_primes) / sizeof(int); i++) {
        if (valid_primes[i] >= capacity) {
            return valid_primes[i];
        }
    }
    panic("NO size found greater than %d\n", capacity);
    return -1;
}
