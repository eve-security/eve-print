/*
 * Built from code provided in libfprint/examples
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <libfprint/fprint.h>


const size_t NAME_SIZE = 50;
const char* PATH = "prints";
const char* EXT = "fp";
#define false 0
#define true 1
typedef int bool;


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



void enroll()
{
    int r;
    char name[NAME_SIZE];
    char filepath[BUFSIZ];
    FILE* outFile = NULL;
    bool fileAvaible = false;
    size_t name_end;
    struct fp_print_data *enrolled_print = NULL;
    struct fp_dev* dev = connect(); //TODO move this to main
    
    printf("You are about to add a new fingerprint to the database. Doing so "
            "will require a restart of the verification program.\n");

    printf("Please enter a name for the print. The name of the person owning "
            "the print is usually a good idea.\n");


    while (!fileAvaible)
    {
        printf("Name: ");
        if ( fgets(name, NAME_SIZE, stdin) == NULL  ){
            return;
        }
        name_end = strnlen(name,NAME_SIZE);
        if (name_end < 3) {
            printf("Name must be at least 2 chars\n");
            continue;
        }
        if (name_end > 1)
        {
            name[name_end-1] = '\0'; //repalce newline with NULL
        }
        (void)snprintf(filepath, BUFSIZ, "%s/%s.%s", PATH, name, EXT);
        if (file_exists(filepath))
        {
            printf("%s already exists.\n",name);
        }else{
            fileAvaible = true;
        }
    }

    printf("You will need to successfully scan your finger %d times to "
        "complete the process.\n", fp_dev_get_nr_enroll_stages(dev));

    do {
        //TODO make this and verify portion into a function
        sleep(1);
        printf("\nScan your finger now.\n");
        r = fp_enroll_finger(dev, &enrolled_print);
        if (r < 0) {
            printf("Enroll failed with error %d\n", r);
            return;
        }
        switch (r) {
        case FP_ENROLL_COMPLETE:
            printf("Enroll complete!\n");
            break;
        case FP_ENROLL_FAIL:
            printf("Enroll failed, something wen't wrong :(\n");
            return;
        case FP_ENROLL_PASS:
            printf("Enroll stage passed. Yay!\n");
            break;
        case FP_ENROLL_RETRY:
            printf("Didn't quite catch that. Please try again.\n");
            break;
        case FP_ENROLL_RETRY_TOO_SHORT:
            printf("Your swipe was too short, please try again.\n");
            break;
        case FP_ENROLL_RETRY_CENTER_FINGER:
            printf("Didn't catch that, please center your finger on the "
                "sensor and try again.\n");
            break;
        case FP_ENROLL_RETRY_REMOVE_FINGER:
            printf("Scan failed, please remove your finger and then try "
                "again.\n");
            break;
        }
    } while (r != FP_ENROLL_COMPLETE);

    if (!enrolled_print) {
        fprintf(stderr, "Enroll complete but no print?\n");
        return;
    }

    printf("Verify that the read was good\n");
    if (!verify(dev,enrolled_print)) {
        //TODO free things
        return;
    }


    //get a buffer containing the raw data to save to disk
    unsigned char *buf;
    size_t len;
    
    len = fp_print_data_get_data(enrolled_print, &buf);
    if (!len) {
        fprintf(stderr,"Unable to get data buffer for print");
    }

    //save enrolled_print
    outFile = fopen(filepath,"wb");
    if (outFile == NULL)
    {
        fprintf(stderr,"Unable to open %s for writing.\n",filepath);
        return;
    }
    if (fwrite(buf, len, 1, outFile) != 1)
    {
        fprintf(stderr,"Error while writing to %s.\n",filepath);
        //TODO close handle and free memory
        return;
    }

    printf("Enrollment completed!\n\n");
    free(buf);
    fp_print_data_free(enrolled_print);
    fp_dev_close(dev);
    fclose(outFile);
}


int main(int argc, char** argv)
{
    int c;
    create_dir(PATH);
    if (argc > 1)
    {
        while ((c = getopt (argc, argv, "eh")) != -1)
        {
            switch (c)
            {
            case 'e':
                enroll();
                return 0;
                //break;
            case '?':
                printf("Unrecognized option -%c\n",optopt);
            case 'h':
                printf("Usage:\n");
                printf("%s -h\tDisplay this message\n",argv[0]);
                printf("%s -e\tEnroll a new print\n",argv[0]);
                printf("%s \t\tScan for prints\n",argv[0]);
                return 0;
                //break;
            }
        }
    }else{
        //verify();
    }
    return 0;
}