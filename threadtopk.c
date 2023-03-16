#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_SIZE 64
#define MAX_NO_OF_WORDS 1000
#define MAX_NO_OF_FILES 10


char** inputFileNames;

typedef struct {
    char word[MAX_WORD_SIZE];
    int countNum;
} WordCount;

WordCount* resultsPointer;

struct arg {
    /* name of the file that is processed */
    char fileName[MAX_WORD_SIZE];
    /* the 'k' in the top-k word with the highest frequency */
    int k;
    /* the index of the created thread */
	int t_index;
};

int compareWordCountFreq(const void* wordCount1, const void* wordCount2){
    const WordCount* word1 = (const WordCount*) wordCount1;
    const WordCount* word2 = (const WordCount*) wordCount2;
    return word2->countNum - word1->countNum;
}

/* The function to be executed concurrently by all the threads */
static void *processFile(void *arg_ptr)
{
    int t_index = ((struct arg *) arg_ptr)->t_index;
    int k = ((struct arg *) arg_ptr)->k;
    FILE* filePtr;
    filePtr = fopen(((struct arg *) arg_ptr)->fileName, "r");

    if(filePtr == NULL)
    {
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

    int unfilledKVals = numberOfWords;
    while(unfilledKVals < k){
        WordCount wordNew; 
        strcpy(wordNew.word, "");
        wordNew.countNum = 0;
        wordsAccessed[unfilledKVals] = wordNew;
        unfilledKVals++;
    }

    /* sort the word accessed struct array in descending order */
    qsort(wordsAccessed,numberOfWords,sizeof(WordCount),compareWordCountFreq);

    /* write the top-k words into the shared memory */
    for(int wordIndex = 0; wordIndex < k; wordIndex++){
        memcpy((resultsPointer + (t_index * k) + wordIndex), &wordsAccessed[wordIndex], sizeof(WordCount));
        printf("Shared Memory Write -> Address: %p, Word: %s, Count: %d\n", 
        (resultsPointer + (t_index * k) + wordIndex), (resultsPointer + (t_index * k) + wordIndex)->word, 
        (resultsPointer + (t_index * k) + wordIndex)->countNum);
    }
    fclose(filePtr);
    pthread_exit(NULL);
}


int main(int argc, char* argv[])
{
    /* the thread ids */
    pthread_t tids[MAX_NO_OF_FILES];
    /* the number of threads */
    int numOfInputFiles;
    /* thread function arguments (the parameters to the function) */
    struct arg t_args[MAX_NO_OF_FILES];
    /* the 'k' in the top-k words with the higest frequency */ 
    int k;
    /* the name of the output file */
    char* outfile;

    int ret;

    if(argc < 5)
    {
        exit(1);
    }

    k = atoi(argv[1]);
    outfile = argv[2];
    numOfInputFiles = atoi(argv[3]);
    inputFileNames = &argv[4];

    WordCount threadResults[numOfInputFiles][k];
    resultsPointer = (WordCount*) &threadResults;

    for(int tIndex = 0; tIndex < numOfInputFiles; tIndex++){
        strcpy(t_args[tIndex].fileName, inputFileNames[tIndex]);
        t_args[tIndex].k = k;
        t_args[tIndex].t_index = tIndex;

        ret = pthread_create(&(tids[tIndex]),
				     NULL, processFile, (void *) &(t_args[tIndex]));

        if (ret != 0){
			exit(1);
        }
    }

	for (int tIndex = 0; tIndex < numOfInputFiles; tIndex++) {
	    ret = pthread_join(tids[tIndex], NULL);
		if (ret != 0) {
			exit(1);
		}
	}

    int wordsProcessedSize = numOfInputFiles * k; 
    WordCount wordsProcessed[wordsProcessedSize];
    int wordsProcessedNum = 0;

    for(int threadIndex = 0; threadIndex < numOfInputFiles; threadIndex++){
        for(int arrIndex = 0; arrIndex < k; arrIndex++){
            if(threadResults[threadIndex][arrIndex].countNum <= 0){
                continue;
            }
            printf("Shared Memory Read -> Address: %p, Word: %s, Count: %d\n", 
            &threadResults[threadIndex][arrIndex], threadResults[threadIndex][arrIndex].word, 
            threadResults[threadIndex][arrIndex].countNum);
            int isWordExist = 0;

            for(int j = 0; j < wordsProcessedNum; j++){
                if(strcmp(threadResults[threadIndex][arrIndex].word,wordsProcessed[j].word) == 0){
                    isWordExist = 1;
                    wordsProcessed[j].countNum += threadResults[threadIndex][arrIndex].countNum;
                    break;
                }
            }
            if(isWordExist == 0) {
                memcpy(&wordsProcessed[wordsProcessedNum], &threadResults[threadIndex][arrIndex], sizeof(WordCount));
                wordsProcessedNum++;
            }
        }
    }

    /* sort the words processed struct array in descending order */
    qsort(wordsProcessed,wordsProcessedNum,sizeof(WordCount),compareWordCountFreq);

    FILE* out = fopen(outfile, "w");

    for(int i = 0; i < k; i++){
        fprintf(out, "%s", wordsProcessed[i].word);
        fprintf(out, " %d\n", wordsProcessed[i].countNum);
    }

    fclose(out); 
    
	exit(0);
    return(0);
}