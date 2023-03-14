#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define MAX_WORD_SIZE 64

typedef struct {
    char word[MAX_WORD_SIZE];
    int countNum;
} WordCount;

/* array of input files' names */
char** inputFileNames;
/* name of the shared memory */
char* shmName;


void toUpperCase(char* str) {
    for(int i = 0; str[i] != '\0'; i++){
        str[i] = str[i] - 32;
    }
}

void processFile(char* fileName, void *shmPosition, int k) {

}

int main(int argc, char *argv[]){
    pid_t pid_n;
    /* number of highest frequency words to be found in a file */
    int k;
    /* name of the output file */
    char* outfile;
    /* N: number of input files */
    int numOfInputFiles;
    /* file descriptor of the shared memory */
    int shmFd;
    /* size of the shared memory size */
    size_t shmSize;
    /* pointer to the shared memory object */
    WordCount *shmStart; 

    /* extractring the command line arguments */
    if (argc < 5){
        exit(1);
    }
    k = atoi(argv[1]);
    outfile = argv[2];
    numOfInputFiles = atoi(argv[3]);
    inputFileNames = &argv[4];

    /*create a shared memory segment*/
    shmFd = shm_open(shmName, O_RDWR | O_CREAT, 0666);
    if (shmFd < 0) {
        exit(1);
    }

    /*set the size of the shared memory*/
    shmSize = k * numOfInputFiles * sizeof(WordCount);
    ftruncate(shmFd, shmSize);

    /* map the shared memory into the address space of the parent process */
    shmStart = (WordCount*) mmap(0, shmSize, PROT_READ | PROT_WRITE, 
                    MAP_SHARED, shmFd, 0);
    if(shmStart < 0) {
        exit(1);
    }

    for(int fileNum = 0; fileNum < numOfInputFiles; fileNum++){
        pid_n = fork();
        if (pid_n < 0) {
            exit(1); 
        }
        if( pid_n == 0) {
            void* shmPosition = shmStart;
            if(fileNum != 0){
                shmPosition += fileNum + k; 
            }
            char* fileName = (char*) (inputFileNames + fileNum);
            processFile(fileName, shmPosition, k);
            exit(0);
        }
    }
    //parent process

}
