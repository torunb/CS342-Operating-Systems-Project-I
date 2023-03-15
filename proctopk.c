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
#define MAX_NO_OF_WORDS 1000

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

int compareWordCountFreq(const WordCount* wordCount1, const WordCount* wordCount2){
    return wordCount1->countNum - wordCount2->countNum;
}

void processFile(char* fileName, void *shmPosition, int k) {
    FILE* filePtr;
    filePtr = fopen(fileName, "r");

    if(filePtr == NULL)
    {
        printf("File cannot be opened: %s\n", fileName);
        exit(1);
    }

    /* the scanned words and their frequencies */
    WordCount wordsAccessed[MAX_NO_OF_WORDS];
    int numberOfWords = 0;

    /* the word that is currently scanned */
    char currentWord[MAX_WORD_SIZE];

    while(fscanf(filePtr, "%s", currentWord) != EOF)
    {
        /* the boolean that checks whether the word is previously scanned */
        int isPreviouslyScanned = 0;

        /* convert word to all upper case */
        toUpperCase(currentWord);

        for(int scannedWordIndex= 0; scannedWordIndex < numberOfWords; scannedWordIndex++ ){
            if(strcmp(wordsAccessed[scannedWordIndex].word, currentWord) == 0) {
                wordsAccessed[scannedWordIndex].countNum++;
                isPreviouslyScanned = 1;
                break;
            }
        }

        if(isPreviouslyScanned == 0){
            WordCount wordNew; 
            strcpy(wordNew.word, currentWord);
            wordNew.countNum = 1;
            wordsAccessed[numberOfWords] = wordNew;
            numberOfWords++;
        }
    }

    /* sort the word accessed struct array */
    qsort(wordsAccessed,numberOfWords,sizeof(WordCount),compareWordCountFreq);

    
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
