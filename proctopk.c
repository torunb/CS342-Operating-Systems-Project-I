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
#include <ctype.h>
#

#define MAX_WORD_SIZE 64
#define MAX_NO_OF_WORDS 1000
#define MAX_NO_OF_FILES 10

typedef struct {
    char word[MAX_WORD_SIZE];
    int countNum;
} WordCount;

/* array of input files' names */
char** inputFileNames;
/* name of the shared memory */
char* shmName;

int compareWordCountFreq(const void* wordCount1, const void* wordCount2){
    const WordCount* word1 = (const WordCount*) wordCount1;
    const WordCount* word2 = (const WordCount*) wordCount2;
    return word2->countNum - word1->countNum;
}

void processFile(char* fileName, WordCount *shmPosition, int k) {
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
         for(int i = 0; i < strlen(currentWord); i++)
        {
            currentWord[i] = toupper(currentWord[i]); // make all the words upper case
        }

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

    /* sort the word accessed struct array in descending order */
    qsort(wordsAccessed,numberOfWords,sizeof(WordCount),compareWordCountFreq);

    /* write the top-k words into the shared memory */
    for(int wordIndex = 0; wordIndex < k; wordIndex++){
        shmPosition += wordIndex; 
        memcpy(shmPosition, &wordsAccessed[wordIndex], sizeof(WordCount));
    }
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
            WordCount* shmPosition = shmStart + (k * fileNum);
            char* fileName = inputFileNames[fileNum];
            processFile(fileName, shmPosition, k);
            exit(0);
        }
    }
    
    //wait all child processes to terminate
    for(int i = 0; i < numOfInputFiles; i++){
        wait(NULL);
    }

    int wordsProcessedSize = MAX_NO_OF_FILES * MAX_NO_OF_WORDS; 
    WordCount wordsProcessed[wordsProcessedSize];
    int wordsProcessedNum = 0;
    WordCount* shmPosition = shmStart;

    for(int procWordIndex = 0; procWordIndex < (numOfInputFiles * k); procWordIndex++){
        int isWordExist = 0;
        shmPosition += procWordIndex;
        for(int j = 0; j < wordsProcessedNum; j++){
            if(strcmp(shmPosition->word,wordsProcessed[j].word) == 0){
                isWordExist = 1;
                wordsProcessed[j].countNum += shmPosition->countNum;
                break;
            }
        }
        if(isWordExist == 0) {
            memcpy(&wordsProcessed[procWordIndex], shmPosition, sizeof(WordCount));
            wordsProcessedNum++;
        }
    }

    /* sort the words processed struct array in descending order */
    qsort(wordsProcessed, wordsProcessedNum,sizeof(WordCount),compareWordCountFreq);

    FILE* out = fopen(outfile, "w");

    char word[MAX_WORD_SIZE] ; 

    for(int i = 0; i < k; i++){
        fprintf(out, "%s", wordsProcessed[i].word);
        fprintf(out, " %d\n", wordsProcessed[i].countNum);
    }

    fclose(out);

    close(shmFd);
    shm_unlink(shmName);

    exit(0);
    return(0);
}
