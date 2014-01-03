#include "finger_auth.h"


bool create_dir(const char * path)
{
    int e;
    struct stat sb;

    e = stat(path, &sb);
    if (e == 0)
    {
        if (sb.st_mode & S_IFDIR) {
            return true;
        }else{
            fprintf(stderr,"Can't create dir, %s exists and is not a dir.\n",path);
            return false;
        }
    }else{
        if (errno == ENOENT) {
            e = mkdir(path, S_IRWXU);
            if (e == 0) {
                return true;
            }
        }
    }
    fprintf(stderr,"Error creating dir: %s\n",path);
    return false;
}


bool file_exists(const char * filename)
{
    FILE* file;
    if ((file = fopen(filename, "r")) != NULL)
    {
        fclose(file);
        return true;
    }
    return false;
}


struct fp_dscv_dev *discover_device(struct fp_dscv_dev **discovered_devs)
{
    struct fp_dscv_dev *ddev = discovered_devs[0];
    struct fp_driver *drv;
    if (!ddev)
        return NULL;

    drv = fp_dscv_dev_get_driver(ddev);
    printf("Found device claimed by %s driver\n", fp_driver_get_full_name(drv));
    return ddev;
}


struct fp_dev* connect()
{
    int r = 1;
    struct fp_dscv_dev *ddev;
    struct fp_dscv_dev **discovered_devs;
    struct fp_dev *dev;

    r = fp_init();
    if (r < 0) {
        fprintf(stderr, "Failed to initialize libfprint\n");
        exit(1);
    }
    fp_set_debug(3);

    discovered_devs = fp_discover_devs();
    if (!discovered_devs) {
        fprintf(stderr, "Could not discover devices\n");
        fp_exit();
        exit(1);
    }

    ddev = discover_device(discovered_devs);
    if (!ddev) {
        fprintf(stderr, "No devices detected.\n");
        fp_exit();
        exit(1);
    }

    dev = fp_dev_open(ddev);
    fp_dscv_devs_free(discovered_devs);
    if (!dev) {
        fprintf(stderr, "Could not open device.\n");
        fp_exit();
        exit(1);
    }
    return dev;
}


struct fp_print_data * load_print_from_file(const char * path)
{
    FILE * fh;
    size_t fSize;
    static unsigned char buffer[FILE_BUFFER_SIZE];
    
    fh = fopen(path,"rb");
    if (!fh) {
        fprintf(stderr,"Error reading: %s\n",path);
        return NULL;
    }

    //clear buffer
    memset(&buffer,0,FILE_BUFFER_SIZE);

    fSize = fread(&buffer,1,FILE_BUFFER_SIZE,fh);
    fclose(fh);

    if (fSize < 1){
        fprintf(stderr,"Empty File: %s\n",path);
        return NULL;
    }

    return fp_print_data_from_data(&(buffer[0]),fSize);
}


void loadPrints(struct user_prints * users)
{
    DIR * d;
    struct dirent *dir;
    struct fp_print_data * print;
    struct fp_print_data ** prints_temp;
    char ** names_temp;
    char filepath[BUFSIZ];
    size_t fLen;

    users->names = NULL;
    users->prints = NULL;
    users->length = 0;

    d = opendir(PATH);
    if (d) {
        while ((dir = readdir(d)) != NULL)
        {
            fLen = strnlen(dir->d_name,NAME_SIZE+3);
            if ( fLen < 4) {
                continue;
            }
            //test extension
            if ( strncmp( &(dir->d_name[fLen-2]), EXT,2)) {
                continue;
            }
            //read file
            (void)snprintf(filepath, BUFSIZ, "%s/%s", PATH, dir->d_name);
            print = load_print_from_file(filepath);

            if (print == NULL) {
                fprintf(stderr,"Unable to parse %s to print\n",dir->d_name);
                continue;
            }

            //resize array
            names_temp = users->names;
            prints_temp = users->prints;
            users->names = realloc(names_temp,(users->length+1)*sizeof(char*));
            users->prints = realloc(prints_temp,(users->length+1)*sizeof(struct fp_print_data *));

            //move name
            users->names[users->length] = malloc(NAME_SIZE*sizeof(char));
            strncpy(users->names[users->length],dir->d_name,fLen-3);
            users->names[users->length][fLen-3] = '\0'; //ensure null terminated string

            //convert and load print data
            users->prints[users->length] = print;
            users->length++;
        }
        closedir(d);

        //null terminate prints
        prints_temp = users->prints;
        users->prints = realloc(prints_temp,(users->length+1)*sizeof(struct fp_print_data *));
        users->prints[users->length] = NULL;
    }
}


//scan a print and verify that it matches the provided print
bool verify(struct fp_dev* dev, struct fp_print_data *data)
{
    int r;
    do {
        sleep(1);
        printf("\nScan your finger now.\n");
        r = fp_verify_finger(dev, data);
        if (r < 0) {
            printf("verification failed with error %d :(\n", r);
            return false;
        }
        switch (r) {
            case FP_VERIFY_NO_MATCH:
                printf("NO MATCH!\n");
                return false;
            case FP_VERIFY_MATCH:
                printf("MATCH!\n");
                return true;
            case FP_VERIFY_RETRY:
                printf("Scan didn't quite work. Please try again.\n");
                break;
            case FP_VERIFY_RETRY_TOO_SHORT:
                printf("Swipe was too short, please try again.\n");
                break;
            case FP_VERIFY_RETRY_CENTER_FINGER:
                printf("Please center your finger on the sensor and try again.\n");
                break;
            case FP_VERIFY_RETRY_REMOVE_FINGER:
                printf("Please remove finger from the sensor and try again.\n");
                break;
        }
    } while (1);

    return true;
}

int identify(struct fp_dev * dev, struct user_prints * users)
{
    size_t id;
    int r;

    do {
        sleep(1);
        printf("\nScan your finger now.\n");
        r = fp_identify_finger(dev, users->prints,&id);
        if (r < 0) {
            printf("verification failed with error %d :(\n", r);
            return -1;
        }
        switch (r) {
            case FP_VERIFY_NO_MATCH:
                printf("NO MATCH!\n");
                return -1;
                break;
            case FP_VERIFY_MATCH:
                printf("MATCH!\n");
                return id;
            case FP_VERIFY_RETRY:
                printf("Scan didn't quite work. Please try again.\n");
                break;
            case FP_VERIFY_RETRY_TOO_SHORT:
                printf("Swipe was too short, please try again.\n");
                break;
            case FP_VERIFY_RETRY_CENTER_FINGER:
                printf("Please center your finger on the sensor and try again.\n");
                break;
            case FP_VERIFY_RETRY_REMOVE_FINGER:
                printf("Please remove finger from the sensor and try again.\n");
                break;
        }
    } while (true);
    return -1;
}


