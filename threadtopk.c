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
	int min;			// min value
	int max;			// max value
	int t_index;    // the index of the created thread
};

int compareWordCountFreq(const void* wordCount1, const void* wordCount2){
    const WordCount* word1 = (const WordCount*) wordCount1;
    const WordCount* word2 = (const WordCount*) wordCount2;
    return word2->countNum - word1->countNum;
}

void processFile(char* fileName, int k)
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
    pthread_t tids[MAX_NO_OF_FILES]; // thread ids

    int numOfInputFiles;
    int k;
    char* outputFile; // name of the output file
    
    struct arg t_args[MAX_NO_OF_FILES];

    int ret;

    if(argc < 5)
    {
        exit(1);
    }

    k = atoi(argv[1]);
    numOfInputFiles = atoi(argv[3]);
    outputFile = argv[2];
    inputFileNames = &argv[4];

    int numberOfThreads = numOfInputFiles;

    for(int i = 0; i < numberOfThreads; i++)
    {

    }


}