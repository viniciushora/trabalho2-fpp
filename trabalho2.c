#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
	int* numbers;
	int max;
	int bufferSize;
	int count;
	int resourcesGenerated;
	int maxCount;
	int primeCount;
	int front;
	int rear;
	int round;
	int actualNumber;
	int quantityThreads;
	sem_t* semThreads;
	sem_t bufferEnd;
	pthread_mutex_t mutex;
	pthread_cond_t full;
	pthread_cond_t empty;
} PipelineBuffer;

typedef struct {
	int count;
	int max;
	PipelineBuffer* pipeline;
} GeneratorThread;

typedef struct {
	int number;
	int id;
	int divisor;
	int round;
} PrinterPackage;

typedef struct {
	PrinterPackage* inputs;
	sem_t printEnd;
	int lastPrint;
	int deadIndex;
	int count;
	int length;
	pthread_mutex_t mutex;
	sem_t printVerifier;
	sem_t printMailer;
} PrinterBuffer;

typedef struct {
	PrinterBuffer* buffer;
} PrinterThread;

typedef struct {
	int id;
	int* primes;
	int primeCount;
	PipelineBuffer* pipeline;
	PrinterBuffer* printerBuffer;
} AnalyzeThread;

void initializeBuffer(PipelineBuffer* buffer, int maximumSize, int quantityThreads, int bufferSize, sem_t* semThreads) {
	buffer->numbers = (int*)malloc(bufferSize * sizeof(int));
	buffer->max = maximumSize;
	buffer->bufferSize = bufferSize;
	buffer->quantityThreads = quantityThreads;
	buffer->count = 0;
	buffer->resourcesGenerated = 0;
	buffer->maxCount = 0;
	buffer->primeCount = 0;
	buffer->front = 0;
	buffer->rear = 0;
	buffer->round = 0;
	buffer->actualNumber = 0;
	sem_init(&buffer->bufferEnd, 0, 0);
	pthread_mutex_init(&buffer->mutex, NULL);
	pthread_cond_init(&buffer->full, NULL);
	pthread_cond_init(&buffer->empty, NULL);

	sem_init(&semThreads[0], 0, 1);
	for (int i = 1; i < quantityThreads; i++) {
		sem_init(&semThreads[i], 0, 0);
	}
	buffer->semThreads = semThreads;
}

void initializeThread(int id, PipelineBuffer* pipeline, PrinterBuffer* printerBuffer, int primesSize, pthread_t* analyzer) {
	AnalyzeThread analyzerArgs;
	analyzerArgs.id = id;
	analyzerArgs.pipeline = pipeline;
	analyzerArgs.primes = (int*)malloc(primesSize * sizeof(int));
	analyzerArgs.printerBuffer = printerBuffer;
	analyzerArgs.primeCount = 0;
	//pthread_create(&analyzer[id], NULL, analyzeNumbers, (void*)&analyzerArgs);
}

void initializePrinterBuffer(PrinterBuffer* buffer, int maximumSize) {
	buffer->inputs = (PrinterPackage*)malloc(maximumSize * sizeof(PrinterPackage));
	buffer->length = maximumSize;
	sem_init(&buffer->printEnd, 0, 0);
	buffer->deadIndex = 0;
	buffer->lastPrint = 1;
	buffer->count = 0;
	pthread_mutex_init(&buffer->mutex, NULL);
	sem_init(&buffer->printMailer, 0, 1);
	sem_init(&buffer->printVerifier, 0, 0);
}

void destroyBuffer(PipelineBuffer* buffer) {
	free(buffer->numbers);
	pthread_mutex_destroy(&buffer->mutex);
	pthread_cond_destroy(&buffer->full);
	pthread_cond_destroy(&buffer->empty);
}

void produceNumber(PipelineBuffer* buffer, int number) {

	pthread_mutex_lock(&buffer->mutex);

	while (buffer->count == buffer->bufferSize) {
		pthread_cond_wait(&buffer->empty, &buffer->mutex);
	}

	buffer->numbers[buffer->rear] = number;
	buffer->rear = (buffer->rear + 1) % buffer->bufferSize;
	buffer->count++;
	buffer->resourcesGenerated++;

	pthread_cond_signal(&buffer->full);
	pthread_mutex_unlock(&buffer->mutex);
}

int consumeNumber(PipelineBuffer* buffer) {
	int number;

	pthread_mutex_lock(&buffer->mutex);

	while (buffer->count == 0) {
		pthread_cond_wait(&buffer->full, &buffer->mutex);
	}

	number = buffer->numbers[buffer->front];
	buffer->front = (buffer->front + 1) % buffer->bufferSize;
	buffer->count--;

	pthread_cond_signal(&buffer->empty);
	pthread_mutex_unlock(&buffer->mutex);

	return number;
}

void produceResult(PrinterBuffer* buffer, int number, int id, int round, int divisor) {
	if (divisor == 0) {
		printf("%d is prime in thread %d at round %d\n", number, id, round);
	}
	else if (divisor > 0) {
		printf("%d divided by %d in thread %d at round %d\n", number, divisor, id, round);
	}
	else {
		printf("%d CAUSED INTERNAL BUFFER OVERFLOW IN thread %d at round %d\n", number, id, round);
		sem_post(&buffer->printEnd);
	}
}

void* printResults(void* args) {
	PrinterThread* threadArgs = (PrinterThread*)args;
	PrinterBuffer* buffer = threadArgs->buffer;
	int i;

	while (1) {
		sem_wait(&buffer->printVerifier);
		for (i = 0; i < buffer->length; i++) {
			if (buffer->inputs[i].number == buffer->lastPrint + 1) {
				produceResult(buffer, buffer->inputs[i].number, buffer->inputs[i].id, buffer->inputs[i].round, buffer->inputs[i].divisor);
				buffer->lastPrint++;
				buffer->deadIndex++;
			}
		}
		sem_post(&buffer->printMailer);
	}
}

void sendNumberToPrinter(PrinterBuffer* buffer, PrinterPackage numberPackage) {
	sem_wait(&buffer->printMailer);
	int i = buffer->count;

	buffer->inputs[i] = numberPackage;

	buffer->count++;

	sem_post(&buffer->printVerifier);
}

void* generateNumbers(void* args) {
	GeneratorThread* threadArgs = (GeneratorThread*)args;
	PipelineBuffer* buffer = threadArgs->pipeline;

	int number = 2;

	for (number; number <= buffer->max; number++) {
		produceNumber(buffer, number);
	}
	return NULL;
}

void* analyzeNumbers(void* args) {
	AnalyzeThread* threadArgs = (AnalyzeThread*)args;
	PipelineBuffer* buffer = threadArgs->pipeline;
	PrinterBuffer* printerBuffer = threadArgs->printerBuffer;
	int id = threadArgs->id;
	PrinterPackage numberPackage;
	int number;
	int nextThread;
	int returnToZero = 0;

	while (1) {
		sem_wait(&buffer->semThreads[id]);

		numberPackage.round = buffer->round;
		numberPackage.id = id;

		if (id == buffer->quantityThreads - 1) {
			nextThread = 0;
			returnToZero = 1;
		}
		else {
			nextThread = id+1;
		}

		if (buffer->actualNumber == 0) {
			number = consumeNumber(buffer);
			buffer->actualNumber = number;
		} else {
			number = buffer->actualNumber;
		}

		numberPackage.number = number;

		if (buffer->round + 1 > buffer->bufferSize) {
			numberPackage.divisor = -1;
			sendNumberToPrinter(printerBuffer, numberPackage);
		}

		int isPrime = 1;
		numberPackage.id = id;
		numberPackage.number = number;

		if (number % threadArgs->primes[buffer->round] == 0) {
			isPrime = 0;
			numberPackage.divisor = threadArgs->primes[buffer->round];
		}

		if (isPrime) {
			if (threadArgs->primeCount <= buffer->round) {
				threadArgs->primes[buffer->round] = number;
				threadArgs->primeCount++;

				numberPackage.divisor = 0;

				sendNumberToPrinter(printerBuffer, numberPackage);
				buffer->round = 0;
				buffer->actualNumber = 0;
				sem_post(&buffer->semThreads[0]);
			} else {
				if (returnToZero) {
					buffer->round++;
				}
				sem_post(&buffer->semThreads[nextThread]);
			}
		} else {
			sendNumberToPrinter(printerBuffer, numberPackage);
			buffer->round = 0;
			buffer->actualNumber = 0;
			sem_post(&buffer->semThreads[0]);
		}

		if (number == buffer->maxCount) {
			sem_wait(&buffer->bufferEnd);
		}
	}
}

int main(int argc, char* argv[]) {
	/*
	if (argc < 5) {
		printf("Usage: ./primes <N> <M> <K> <X>\n");
		return 1;
	}

	int N = atoi(argv[1]);
	int M = atoi(argv[2]);
	int K = atoi(argv[3]);
	int X = atoi(argv[4]);
	*/
	int N = 8000;
	int M = 4;
	int K = 3;
	int X = 2;

	sem_t* semThreads = (sem_t*)malloc(M * sizeof(sem_t));

	PipelineBuffer buffer;
	initializeBuffer(&buffer, N, M, X, semThreads);

	PrinterBuffer printerBuffer;
	initializePrinterBuffer(&printerBuffer, N);

	pthread_t generator;
	pthread_t* analyzer = (pthread_t*)malloc(M * sizeof(pthread_t));
	pthread_t printer;

	GeneratorThread generatorArgs = { .count = 0, .max = M, .pipeline = &buffer };
	pthread_create(&generator, NULL, generateNumbers, (void*)&generatorArgs);

	PrinterThread printerArgs = { .buffer = &printerBuffer };
	pthread_create(&printer, NULL, printResults, (void*)&printerArgs);

	AnalyzeThread* analyzerArgs = (AnalyzeThread*)malloc(M * sizeof(AnalyzeThread));
	for (int i = 0; i < M; i++) {
		analyzerArgs[i].id = i;
		analyzerArgs[i].pipeline = &buffer;
		analyzerArgs[i].primes = (int*)malloc(X * sizeof(int));
		analyzerArgs[i].printerBuffer = &printerBuffer;
		analyzerArgs[i].primeCount = 0;
		pthread_create(&analyzer[i], NULL, analyzeNumbers, (void*)(analyzerArgs + i));
		//initializeThread(i, &buffer, &printerBuffer, X, &analyzer);
	}

	sem_wait(&printerBuffer.printEnd);

	destroyBuffer(&buffer);

	return 0;
}