#ifndef _BLACKLIST_H
#define _BLACKLIST_H

void load_lists(const char *cache_path, const char *dir);

int check_blacklist(const char *victim);

int check_whitelist(const char *victim);

#endif