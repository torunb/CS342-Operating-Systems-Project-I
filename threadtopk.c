#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define MAX_WORD_SIZE 64
#define MAX_NO_OF_WORDS 1000
#define MAX_NO_OF_FILES 10


char** inputFileNames;

typedef struct {
    char word[MAX_WORD_SIZE];
    int countNum;
} WordCount;

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
static void *processFile(char* fileName, int k)
{
    FILE* filePtr;
    filePtr = fopen(fileName, "r");

    if(filePtr == NULL)
    {
        exit(1);
    }

    char* currentWord[MAX_WORD_SIZE];
    WordCount wordAccessed[MAX_NO_OF_WORDS]; 
    int numberOfWords = 0;

    while(fscanf(filePtr, "%s", currentWord) != EOF)
    {
        int isScanned = 0;

        for(int i = 0; i < strlen(currentWord); i++)
        {
            currentWord[i] = toupper(currentWord[i]);
        }

        for(int i = 0; i < numberOfWords; i++)
        {
            if(strcmp(wordAccessed[i].word, currentWord) == 0)
            {
                wordAccessed[i].countNum++;
                isScanned = 1;
                break;
            }
        }

        if(isScanned == 0)
        {
            WordCount newWord;
            strcpy(newWord.word, currentWord);
            newWord.countNum = 1;
            wordAccessed[numberOfWords] = newWord;
            numberOfWords++;
        }        
    }

    qsort(wordAccessed,numberOfWords,sizeof(WordCount),compareWordCountFreq);

    // for(int wordIndex = 0; wordIndex < k; wordIndex++){
    //     shmPosition += wordIndex; 
    //     memcpy(shmPosition, &wordsAccessed[wordIndex], sizeof(WordCount));
    // }
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
    char* outputFile;

    int ret;
    char* retmsg;

    if(argc < 5)
    {
        exit(1);
    }

    k = atoi(argv[1]);
    outputFile = argv[2];
    numOfInputFiles = atoi(argv[3]);
    inputFileNames = &argv[4];

    for(int tIndex = 0; tIndex < numOfInputFiles; tIndex++){
        strcpy(t_args[tIndex].fileName, inputFileNames[tIndex]);
        t_args[tIndex].k = k;
        t_args[tIndex].t_index = tIndex;

        ret = pthread_create(&(tids[tIndex]),
				     NULL, processFile, (void *) &(t_args[tIndex]));

        if (ret != 0){
            printf("thread create failed \n");
			exit(1);
        }
        printf("thread %i with tid %u created\n", tIndex,
		       (unsigned int) tids[tIndex]);
    }

    printf("main: waiting all threads to terminate\n");
	for (int tIndex = 0; tIndex < numOfInputFiles; tIndex++) {
	    ret = pthread_join(tids[tIndex], (void **)&retmsg);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(1);
		}
		printf ("thread terminated, msg = %s\n", retmsg);
		// we got the reason as the string pointed by retmsg.
		// space for that was allocated in thread function.
        // now we are freeing the allocated space.
		free (retmsg);
	}

	printf("main: all threads terminated\n");
    //main thread execution 
    
	return 0;
}