#ifndef __FINGER_AUTH_H
#define __FINGER_AUTH_H

#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //for sleep
#include <libfprint/fprint.h>
#include <errno.h>

#define NAME_SIZE 50
#define FILE_BUFFER_SIZE 4096
#define PATH "prints"
#define EXT "fp"
#define false 0
#define true 1
typedef int bool;

struct user_prints {
    struct fp_print_data ** prints;
    char ** names;
    size_t length;
} users;

struct fp_dscv_dev *discover_device(struct fp_dscv_dev **discovered_devs);
struct fp_dev* connect();
struct fp_print_data * load_print_from_file(const char * path);
void loadPrints(struct user_prints * users);
bool verify(struct fp_dev* dev, struct fp_print_data *data);
int identify(struct fp_dev * dev, struct user_prints * users);

bool create_dir(const char * path);
bool file_exists(const char * filename);

#endif
