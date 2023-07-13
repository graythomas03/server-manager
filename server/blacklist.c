
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "blacklist.h"

// Bloom filter macros
#define FALSE_POSITIVE_DESIRED 0.01
#define M_EQ_DENOM 0.4840453013918
#define HASH_FUNC_CNT 7

static uint8_t *lists[3];
static unsigned int usr_tbl_len;
static unsigned int ip4_tbl_len;
static unsigned int ip6_tbl_len;
#define USR_LIST lists[0]
#define IP4_LIST lists[1]
#define IP6_LIST lists[2]
// array of functions to use in hashing
static unsigned int (*hash_func_table[HASH_FUNC_CNT])(const char *) = {

};