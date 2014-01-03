/*
 * Built from code provided in libfprint/examples
 */
#include <stdio.h>
#include <unistd.h> //for sleep

#include "arduino_lock.h"
#include "finger_auth.h"


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
        name[NAME_SIZE-1] = '\0'; //NULL terminate
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



void youShallPass()
{
    arduino_unlock();
}

void youShallNotPass()
{
    arduino_blink();
}

void auth(char* arduino_port)
{
    struct fp_dev* dev = connect();
    int result;

    if (fp_dev_supports_identification(dev) == 0){
        fprintf(stderr,"Device does not suport identification!\n");
        return;
    }

    //load all prints from PATH
    loadPrints(&users);
    //TODO free

    if (users.length < 1) {
        fprintf(stderr,"No prints loaded!\n");
        return;
    }
    printf("Loaded %zu prints\n",users.length);


    arduino_connect(arduino_port);

    do
    {
        result = identify(dev, &users);
        if (result > -1){
            printf("USER: %s\n",users.names[result]);

            youShallPass();
        }else{
            youShallNotPass();
        }
    }while(true);

    arduino_close();
}

void printUsage(char* self)
{
    printf("Usage:\n");
    printf("%s -h\t\t\tDisplay this message\n",self);
    printf("%s -e\t\t\tEnroll a new print\n",self);
    printf("%s -p ArduinoPort\t\tScan for prints\n",self);
    printf("%s -p ArduinoPort -g\tUnlock the Door\n",self);
    printf("%s -p ArduinoPort -b\tBlink the LED\n",self);
    printf("Order of arguments matter!\n");
}


void testUnlock(char* port)
{
    arduino_connect(port);
    sleep(1);
    arduino_unlock();
    sleep(5);
    arduino_close();
}

void testBlink(char* port)
{
    arduino_connect(port);
    sleep(1);
    arduino_blink();
    sleep(1);
    arduino_close();
}


int main(int argc, char** argv)
{
    int c;
    char * arduino_port = NULL;
    create_dir(PATH);
    if (argc > 1)
    {
        while ((c = getopt(argc, argv, "ehp:gb")) != -1)
        {
            switch (c)
            {
                case 'e':
                    enroll();
                    return 0;
                case 'p':
                    arduino_port = optarg;
                    break;
                case 'g':
                    testUnlock(arduino_port);
                    return 0;
                    break;
                case 'b':
                    testBlink(arduino_port);
                    return 0;
                    break;
                case '?':
                    printf("Unrecognized option -%c\n",optopt);
                case 'h':
                    printUsage(argv[0]);
                    return 0;
            }
        }
        if (arduino_port != NULL) {
            auth(arduino_port);
        }else {
            fprintf(stderr,"Missing port\n");
        }
    }else{
        printUsage(argv[0]);
    }
    return 0;
}

