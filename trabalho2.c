#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1 

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Definição da estrutura de pacote
typedef struct {
	int numero;
	int rodada;
} Pacote;

// Definição da estrutura de parâmetros
typedef struct {
	int N;                   // Número total de pacotes a serem gerados
	int M;                   // Número de threads analisadoras
	int K;                   // Tamanho do buffer circular
	int X;                   // Tamanho do buffer interno
	Pacote* buffer_comunicacao; // Buffer circular de comunicação
	sem_t sem_buffer;        // Semáforo para controlar o buffer circular
	sem_t sem_buffer_interno; // Semáforo para controlar o buffer interno
	pthread_mutex_t mutex_resultado; // Mutex para proteger o acesso à impressão dos resultados
	int pacote_atual;        // Índice do pacote atual
	int pacotes_concluidos;  // Contador de pacotes concluídos
} Parametros;

// Função para verificar se um número é primo
int isPrimo(int numero) {
	if (numero < 2)
		return 0;

	for (int i = 2; i <= numero / 2; i++) {
		if (numero % i == 0)
			return 0;
	}
	return 1;
}

// Função para enviar um pacote para o buffer circular
void enviarPacote(Pacote pacote, Parametros* parametros) {
	sem_wait(&(parametros->sem_buffer));
	parametros->buffer_comunicacao[parametros->pacote_atual] = pacote;
	parametros->pacote_atual = (parametros->pacote_atual + 1) % parametros->K;
	sem_post(&(parametros->sem_buffer_interno));
}

// Função para receber um pacote do buffer circular
Pacote receberPacote(Parametros* parametros) {
	sem_wait(&(parametros->sem_buffer_interno));
	Pacote pacote = parametros->buffer_comunicacao[parametros->pacotes_concluidos % parametros->K];
	parametros->pacotes_concluidos++;
	sem_post(&(parametros->sem_buffer));
	return pacote;
}

// Função executada pelas threads analisadoras
void* thread_analisadora(void* arg) {
	Parametros* parametros = (Parametros*)arg;
	int* buffer_interno = malloc(parametros->X * sizeof(int));
	int contador_primos = 0;
	int rodada_atual = -1;

	while (1) {
		Pacote pacote = receberPacote(parametros);

		// Verificar se houve mudança de rodada
		if (pacote.rodada != rodada_atual) {
			contador_primos = 0;
			rodada_atual = pacote.rodada;
		}

		// Verificar se o número é divisível por algum primo da rodada atual
		int primo = 1;
		for (int i = 0; i < contador_primos; i++) {
			if (pacote.numero % buffer_interno[i] == 0) {
				primo = 0;
				break;
			}
		}

		// Se for primo, armazenar no buffer interno e enviar para a thread de resultados
		if (primo) {
			if (contador_primos == parametros->X) {
				// Limite do buffer interno atingido, enviar mensagem de erro
				pthread_mutex_lock(&(parametros->mutex_resultado));
				printf("%d CAUSED INTERNAL BUFFER OVERFLOW IN %d, ROUND %d\n", pacote.numero, pacote.rodada, pthread_self());
				pthread_mutex_unlock(&(parametros->mutex_resultado));
				free(buffer_interno);
				pthread_exit(NULL);
			}
			buffer_interno[contador_primos] = pacote.numero;
			contador_primos++;

			// Enviar resultado para a thread de resultados
			pthread_mutex_lock(&(parametros->mutex_resultado));
			printf("%d is PRIME in %d\n", pacote.numero, pacote.rodada);
			pthread_mutex_unlock(&(parametros->mutex_resultado));
		}
	}
}

// Função executada pela thread geradora de pacotes
void* thread_geradora(void* arg) {
	Parametros* parametros = (Parametros*)arg;

	for (int i = 2; i < parametros->N + 2; i++) {
		Pacote pacote;
		pacote.numero = i;
		pacote.rodada = i - 2;
		enviarPacote(pacote, parametros);
	}

	pthread_exit(NULL);
}

// Função executada pela thread de resultados
void* thread_resultado(void* arg) {
	Parametros* parametros = (Parametros*)arg;

	while (parametros->pacotes_concluidos < parametros->N) {
		Pacote pacote = receberPacote(parametros);

		// Verificar se o número é o próximo na ordem de impressão
		if (pacote.numero == parametros->pacotes_concluidos + 2) {
			pthread_mutex_lock(&(parametros->mutex_resultado));
			printf("%d is NOT PRIME in %d\n", pacote.numero, pacote.rodada);
			pthread_mutex_unlock(&(parametros->mutex_resultado));
		}
		else {
			enviarPacote(pacote, parametros);
		}
	}

	pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
	/*
	if (argc < 5) {
		printf("Número incorreto de argumentos.\n");
		return 1;
	}

	Parametros parametros;
	parametros.N = atoi(argv[1]);
	parametros.M = atoi(argv[2]);
	parametros.K = atoi(argv[3]);
	parametros.X = atoi(argv[4]);
	*/
	Parametros parametros;
	parametros.N = 8000;
	parametros.M = 4;
	parametros.K = 3;
	parametros.X = 2;

	parametros.buffer_comunicacao = malloc(parametros.K * sizeof(Pacote));
	parametros.pacote_atual = 0;
	parametros.pacotes_concluidos = 0;

	sem_init(&(parametros.sem_buffer), 0, parametros.K);
	sem_init(&(parametros.sem_buffer_interno), 0, 0);
	pthread_mutex_init(&(parametros.mutex_resultado), NULL);

	pthread_t thread_geradora_id;
	pthread_t thread_resultado_id;
	pthread_t* thread_analisadora_ids = malloc(parametros.M * sizeof(pthread_t));

	pthread_create(&thread_geradora_id, NULL, thread_geradora, (void*)&parametros);
	pthread_create(&thread_resultado_id, NULL, thread_resultado, (void*)&parametros);

	for (int i = 0; i < parametros.M; i++) {
		pthread_create(&(thread_analisadora_ids[i]), NULL, thread_analisadora, (void*)&parametros);
	}

	pthread_join(thread_geradora_id, NULL);
	pthread_join(thread_resultado_id, NULL);

	for (int i = 0; i < parametros.M; i++) {
		pthread_join(thread_analisadora_ids[i], NULL);
	}

	free(parametros.buffer_comunicacao);
	free(thread_analisadora_ids);

	return 0;
}