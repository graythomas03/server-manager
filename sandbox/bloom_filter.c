#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BIT_ARR_SIZE 60
#define BIT_CNT 480
#define K 7

/**
 * sample hash functions
 */

uint8_t filter[BIT_ARR_SIZE];
// array of hash functions
unsigned int (*hash_funcs[K])(const char *) = {

};

void add_to_filter(const char *key)
{
    for (int i = 0; i < K; ++i)
    {
        int tmp = hash_funcs[i](key);
        int idx = 0;
        while (tmp - 8 >= 0)
        {
            tmp -= 8;
            ++idx;
        }

        uint8_t set = (0x80 >> tmp);
        filter[idx] |= set;
    }
}

bool check(const char *key)
{
    for (int i = 0; i < K; ++i)
    {
        int tmp = hash_funcs[i](key);
        int idx = 0;
        while (tmp - 8 >= 0)
        {
            tmp -= 8;
            ++idx;
        }

        uint8_t set = (0x80 >> tmp);
        if (filter[idx] | set == 0)
            return false;
    }

    return true;
}

int main()
{
}