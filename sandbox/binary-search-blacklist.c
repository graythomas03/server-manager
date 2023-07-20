#include <ctype.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define TYPE_STR_LEN 5
#define UID_STR_LEN 32
#define IP4_STR_LEN 15
#define IP6_STR_LEN 45
#define MAX_TOK_LEN 50

// hacky compiler-neatural 128 bit unsigned int
typedef struct
{
    uint64_t lower;
    uint64_t upper;
} uint128_t;

// blacklist table vars
size_t uid_table_cap = 0;
size_t uid_table_cnt = 0;
uid_t *uid_table;
size_t gid_table_cap = 0;
size_t gid_table_cnt = 0;
gid_t *gid_table;
size_t ip4_table_cap = 0;
size_t ip4_table_cnt = 0;
uint32_t *ip4_table;
size_t ip6_table_cap = 0;
size_t ip6_table_cnt = 0;
uint128_t *ip6_table;

void bst_add(void **table, size_t *table_size, size_t *table_cap, void *new_value, size_t element_size)
{
    if (element_size == 0)
        return;
    // determine if a reallocation is necessary
    if ((*table_size) + 1 >= *table_cap) {
        switch (*table_cap) {
        case 0:
            *table = calloc(1, element_size);
            *table_cap = 1;
            break;
        default:
            *table_cap = (*table_cap << 1) + 1;
            *table = realloc(*table, (*table_cap) * element_size);
        }
    }

    size_t idx = 0, offset = 0;
    void *current = *table;

    // if element size is <= 8 bytes, can treat as a normal variable
    uint64_t tmp;
    uint8_t tmp_0[element_size];

    switch (element_size) {
    case 8:
        tmp = *((uint8_t *)new_value);
        break;
    case 16:
        tmp = *((uint16_t *)new_value);
        break;
    case 32:
        tmp = *((uint32_t *)new_value);
        break;
    case 64:
        tmp = *((uint64_t *)new_value);
        break;
    // otherwise, do it generically and exit
    default:
        // find index where value should be added
        for (; idx < *table_size && memcmp(current, new_value, element_size) < 0; ++idx, current += element_size)
            ;
        // idx now points to where new element should be inserted
        // iterate backwards
        current = *table + (*table_size * element_size);
        void *next = current + element_size;
        for (int i = *table_size; i > idx; --i, current -= element_size, next -= element_size) {
                }
        return;
    }
}

// different methods of bianry search
bool brachless_bs(uint32_t key)
{
    return false;
}

#define BINARY_SEARCH(VICTIM) branchless_bs((VICTIM))

// placeholder function for when no type is given
inline void parse_nothing(const char *str)
{
    return;
}

inline void parse_usr(const char *str)
{
    uid_t new;
    int i = 0;
    char tmp = str[i];
    // if first char is not a lower case letter, it cannot be a username
    if (!islower(tmp)) {
        // if its not a digit either, bad entry
        if (!isdigit(tmp)) {
            printf("Invalid user id: %s\n", str);
            return;
        }
        // get length of string
        int offset = strlen(str);
        if (offset > UID_STR_LEN) {
            printf("Invalid user id: %s\n", str);
            return;
        }
        // convert string to 32-bit unsigned int
        offset *= 8; // sed 8 bits per digit
        new = 0;
        while (tmp != '\0') {
            // if not a digit either, bad entry
            if (!isdigit(tmp)) {
                printf("Invalid user id: %s\n", str);
                return;
            }
            new += (tmp - '0') << offset;
            tmp = str[i++];
        }
    } else {
        // otherwise, assume its a user name
        struct passwd *tmp = getpwnam(str);
        if (tmp == NULL) {
            printf("No user with name: %s\n", str);
            return;
        }
        new = tmp->pw_uid;
    }

    bst_add((void *)&uid_table, &uid_table_cnt, &uid_table_cap, &new, sizeof(uid_t));
}

// functionally same as parse_usr, just with group ids
inline void parse_grp(const char *str)
{
}

inline void parse_ip4(const char *str)
{
}

inline void parse_ip6(const char *str)
{
}

void load_blacklist(const char *path)
{
    int in_square = 0;
    int line = 1;
    FILE *fp = fopen(path, "r");
    void (*parse)(const char *str) = parse_nothing; // parser function
    int ch, tok_idx = 0;                            // indexers
    char tok[IP6_STR_LEN];                          // temp storage for text
    while ((ch = fgetc(fp)) != EOF) {
        switch (ch) {
        case EOF: // if EOF, stop reading from file
            break;
        case '[': // declare type
            if (in_square) {
                printf("Missing matching ']' on line %d\n", line);
                fclose(fp);
                exit(1);
            }
            in_square = 1;
            tok_idx = 0;
            continue;
        case ']': // end type declaration
            if (!in_square) {
                printf("Missing matching '[' on line %d\n", line);
                fclose(fp);
                exit(1);
            }
            tok[tok_idx] = '\0';
            // set up parser according to type
            if (strcmp("usr", tok) == 0)
                parse = parse_usr;
            else if (strncmp("grp", tok, TYPE_STR_LEN) == 0)
                parse = parse_grp;
            else if (strncmp("ip4", tok, TYPE_STR_LEN) == 0)
                parse = parse_ip4;
            else if (strncmp("ip6", tok, TYPE_STR_LEN) == 0)
                parse = parse_ip6;
            else
                parse = parse_nothing; // set to placeholder parser

            in_square = 0; // set scope status
            tok_idx = 0;   // reset token index
            continue;
        // whitespace cases
        // will be treated as the end of an entry. If in_square, cases are ignored and fall through to default
        case '\n':
            ++line;
        case ' ':
        case '\f':
        case '\v':
        case '\t':
            if (!in_square) {
                tok[tok_idx] = '\0';
                parse(tok);
                tok_idx = 0;
            }
        default: // just read in next full token
            // skip any whitespace
            if (tok_idx >= IP6_STR_LEN) {
                printf("Invalid entry at line: %d\n", line);
                break;
            }
            tok[tok_idx++] = fgetc(fp);
            continue;
        }
        break;
    }
    fclose(fp);
}

bool check_blacklist(const char *key)
{
}
