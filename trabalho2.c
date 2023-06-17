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
	int* primes;
	int max;
	int bufferSize;
	int primeSize;
	int count;
	int maxCount;
	int primeCount;
	int front;
	int rear;
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
} PrinterBuffer;

typedef struct {
	PrinterBuffer* buffer;
} PrinterThread;

typedef struct {
	int id;
	int round;
	PipelineBuffer* pipeline;
	PrinterBuffer* printerBuffer;
} AnalyzeThread;

void initializeBuffer(PipelineBuffer* buffer, int maximumSize, int bufferSize, int primeSize) {
	buffer->numbers = (int*)malloc(bufferSize * sizeof(int));
	buffer->primes = (int*)malloc(primeSize * sizeof(int));
	buffer->max = maximumSize;
	buffer->bufferSize = bufferSize;
	buffer->primeSize = primeSize;
	buffer->count = 0;
	buffer->maxCount = 0;
	buffer->primeCount = 0;
	buffer->front = 0;
	buffer->rear = 0;
	sem_init(&buffer->bufferEnd, 0, 0);
	pthread_mutex_init(&buffer->mutex, NULL);
	pthread_cond_init(&buffer->full, NULL);
	pthread_cond_init(&buffer->empty, NULL);
}

void initializePrinterBuffer(PrinterBuffer* buffer, int maximumSize) {
	buffer->inputs = (PrinterPackage*)malloc(maximumSize * sizeof(PrinterPackage));
	buffer->length = maximumSize;
	sem_init(&buffer->printEnd, 0, 0);
	buffer->deadIndex = 0;
	buffer->lastPrint = 1;
	buffer->count = 0;
	pthread_mutex_init(&buffer->mutex, NULL);
}

void destroyBuffer(PipelineBuffer* buffer) {
	free(buffer->numbers);
	free(buffer->primes);
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
		sem_post(buffer->printEnd);
	}
}

void* printResults(void* args) {
	PrinterThread* threadArgs = (PrinterThread*)args;
	PrinterBuffer* buffer = threadArgs->buffer;
	int i;

	while (1) {
		for (i = 0; i < buffer->length; i++) {
			if (buffer->inputs[i].number == buffer->lastPrint + 1) {
				produceResult(buffer, buffer->inputs[i].number, buffer->inputs[i].id, buffer->inputs[i].round, buffer->inputs[i].divisor);
				buffer->lastPrint++;
				buffer->deadIndex++;
			}
		}
	}
}

void sendNumberToPrinter(PrinterBuffer* buffer, PrinterPackage numberPackage) {
	int i = buffer->count;

	buffer->inputs[i] = numberPackage;

	buffer->count++;
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

	while (1) {
		int number = consumeNumber(buffer);
		threadArgs->round = 0;

		int isPrime = 1;
		numberPackage.id = id;
		numberPackage.number = number;

		for (int i = 0; i < buffer->primeCount; i++) {

			if (number % buffer->primes[i] == 0) {
				isPrime = 0;

				numberPackage.divisor = buffer->primes[i];
				numberPackage.round = threadArgs->round;
				break;
			}
			threadArgs->round++;
		}

		if (isPrime) {
			if (buffer->primeCount < buffer->primeSize) {
				buffer->primes[buffer->primeCount++] = number;

				numberPackage.divisor = 0;
				numberPackage.round = threadArgs->round;

				sendNumberToPrinter(printerBuffer, numberPackage);
			}
			else {
				numberPackage.divisor = -1;
				numberPackage.round = threadArgs->round;
			}
		}

		pthread_mutex_lock(&printerBuffer->mutex);

		sendNumberToPrinter(printerBuffer, numberPackage);

		pthread_mutex_unlock(&printerBuffer->mutex);

		if (number == buffer->maxCount) {
			sem_post(&buffer->bufferEnd);
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

	PipelineBuffer buffer;
	initializeBuffer(&buffer, N, K, X);

	PrinterBuffer printerBuffer;
	initializePrinterBuffer(&printerBuffer, N);

	pthread_t generator;
	pthread_t* analyzer = (pthread_t*)malloc(M * sizeof(pthread_t));
	pthread_t printer;

	sem_t generateEnd;
	sem_init(&generateEnd, 0, 0);

	GeneratorThread generatorArgs = { .count = 0, .max = M, .pipeline = &buffer };
	pthread_create(&generator, NULL, generateNumbers, (void*)&generatorArgs);

	PrinterThread printerArgs = { .buffer = &printerBuffer };
	pthread_create(&printer, NULL, printResults, (void*)&printerArgs);

	for (int i = 0; i < M; i++) {
		AnalyzeThread analyzerArgs = { .id = i, .pipeline = &buffer, .printerBuffer = &printerBuffer };
		pthread_create(&analyzer[i], NULL, analyzeNumbers, (void*)&analyzerArgs);
	}

	sem_wait(&printerBuffer.printEnd);

	destroyBuffer(&buffer);

	return 0;
}