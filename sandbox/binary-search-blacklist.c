#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// array ordering/searching methods
#define CACHE_FRIENDLY 0
#define ORDERED 1
#define MODE ORDERED

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
size_t gid_table_cap = 0;
size_t ip4_table_cap = 0;
size_t ip6_table_cap = 0;
size_t uid_table_cnt = 0;
size_t gid_table_cnt = 0;
size_t ip4_table_cnt = 0;
size_t ip6_table_cnt = 0;
uid_t *uid_table;
gid_t *gid_table;
uint32_t *ip4_table;
uint128_t *ip6_table;

/**
 * Converts string to an unsigned 32-bit integer
 */
inline uint32_t stou32(const char *str)
{
    int idx = 0;
    char ch = *str;
    uint32_t out = 0, tmp;
    while (ch != '\0') {
        if (idx >= 9 || !isdigit(ch))
            return 0;
        // multiply by 10 trick i cobbled together
        out = out << 1;
        tmp = out;
        out = out << 2;
        out += tmp;
        // add new character value
        out += ch - '0';
    }

    return out;
}

#if MODE == ORDERED
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
            *table_cap = (*table_cap << 1) + 1; // double capacity
            *table = realloc(*table, (*table_cap) * element_size);
        }
    }

    // necesary vars
    void *tbl = *table;
    size_t tbl_size = *table_size;
    size_t idx = 0;
    void *current;

    current = *table;
    // insert the new element in its sorted order
    // find index where value should be added
    for (; idx < tbl_size && memcmp(current, new_value, element_size) < 0; ++idx, current += element_size)
        ;
    // idx now points to where new element should be inserted
    // iterate backwards
    current = tbl + (tbl_size * element_size);
    void *prev = current - element_size;
    for (int i = tbl_size; i > idx; --i, current -= element_size, prev -= element_size)
        memcpy(current, prev, element_size);
    memcpy(current, new_value, element_size);

    (*table_size)++;
}
#elif MODE == CACHE_FRIENDLY
void bst_add(void **table, size_t *table_size, size_t *table_cap, void *new_value, size_t element_size)
{
}
#endif

// different methods of bianry search
bool branchless_bs(void *table, void *key, size_t element_size)
{
    return false;
}

#define BINARY_SEARCH(TABLEPTR, VALUEPTR, VALUESIZE) branchless_bs((TABLEPTR), (VALUEPTR), (VALUESIZE))

// placeholder function for when no type is given
inline void parse_nothing(const char *str) { return; }
// convert string into user id table entry
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
        new = stou32(str);
        if (!new) {
            printf("Invalid user id: %s\n", str);
            return;
        }
    } else {
        // otherwise, assume its a user name
        struct passwd *temp = getpwnam(str);
        if (temp == NULL) {
            printf("No user with name: %s\n", str);
            return;
        }
        new = temp->pw_uid;
    }

    bst_add((void *)&uid_table, &uid_table_cnt, &uid_table_cap, &new, sizeof(uid_t));
}

// functionally same as parse_usr, just with group ids
inline void parse_grp(const char *str)
{
    gid_t new;
    int i = 0;
    char tmp = str[i];
    // if first char is not a lower case letter, it cannot be a groupname
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
        // otherwise, assume its a group name
        struct group *temp = getgrnam(str);
        if (temp == NULL) {
            printf("No user with name: %s\n", str);
            return;
        }
        new = temp->gr_gid;
    }

    bst_add((void *)&gid_table, &gid_table_cnt, &gid_table_cap, &new, sizeof(gid_t));
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

// test case driver
int main(int argc, char *argv[])
{
    // take 1st cl arg as blacklist file path
    // take 2nd cl arg as values to test searching runtimes
    if (argc != 2) {
        printf("use: bl-test [blacklist file] [search testing file]");
        exit(1);
    }

    load_blacklist(argv[1]);

    // each value is expected to be seperated by newlines
    FILE *tfp = fopen(argv[2], "r");
    // parsing vars
    int tok_idx = 0;
    char tok[MAX_TOK_LEN];
    int ch;
    // operation vars
    size_t value_size = 0;
    void *table;
    while ((ch = fgetc(tfp)) != EOF) {
        switch (ch) {
        // whitespace or comma used as delimeter
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case ',':
            // skip any additional whitespace
            do {
                ch = fgetc(tfp);
                if (ch == EOF) {
                    ungetc(ch, tfp);
                    break;
                }

            } while (isspace(ch));
            ungetc(ch, tfp);
            // mark as end of token
            tok[tok_idx] = '\0';

        case ':':
            tok[tok_idx] = '\0';
            tok_idx = 0; // reset token indexer
            if (strcmp("uid", tok) == 0) {
                table = uid_table;
                value_size = sizeof(uid_t);
            } else if (strcmp("gid", tok) == 0) {
                table = gid_table;
                value_size = sizeof(gid_t);
            } else if (strcmp("ip4", tok) == 0) {
                table = ip4_table;
                value_size = sizeof(uint32_t);
            } else if (strcmp("ip6", tok) == 0) {
                table = ip6_table;
                value_size = sizeof(uint128_t);
            }

            // skip any additional whitespace
            do {
                ch = fgetc(tfp);
                if (ch == EOF) {
                    ungetc(ch, tfp);
                    break;
                }

            } while (isspace(ch));
            ungetc(ch, tfp);

            break;
        default:
            tok[tok_idx++] = ch;
        }
    }

    fclose(tfp);
}