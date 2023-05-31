# Pacotes em C com Pthreads e Structs

Este tutorial irá guiá-lo através dos passos necessários para criar um programa C que utilize a biblioteca pthreads para passar parâmetros para threads usando uma struct.

## Passo 1
### Incluir as bibliotecas necessárias
Comece incluindo as bibliotecas necessárias para usar pthreads no seu programa. Você precisará incluir a biblioteca <pthread.h>.

```c
#include <pthread.h>
```

## Passo 2
### Definir a struct de parâmetros
Agora, você precisa definir uma struct para representar os parâmetros que serão passados para a thread. Essa struct pode conter qualquer tipo de dado que você desejar. Vamos definir uma struct simples que contém um inteiro e uma string:

```c
typedef struct {
    int numero;
    char mensagem[100];
} Parametros;
```
## Passo 3
### Definir a função da thread
Em seguida, você precisa definir a função que será executada pela thread. Essa função receberá a struct de parâmetros como argumento. Vamos definir uma função simples que imprime o número e a mensagem contidos na struct:

```c
void* minhaThread(void* arg) {
    Parametros* params = (Parametros*) arg;
    printf("Número: %d\n", params->numero);
    printf("Mensagem: %s\n", params->mensagem);
    pthread_exit(NULL);
}
```

## Passo 4
### Criar e executar a thread
Agora, você pode criar e executar a thread principal no seu programa. Dentro da função main(), crie uma variável do tipo pthread_t para representar a thread e uma variável da struct Parametros para armazenar os valores que serão passados para a thread. Preencha a struct com os valores desejados e chame a função <pthread_create()> para criar a thread, passando a struct como argumento.


```c
int main() {
    pthread_t thread;
    Parametros params;
    params.numero = 42;
    strcpy(params.mensagem, "Olá da minha thread!");

    pthread_create(&thread, NULL, minhaThread, (void*) &params);
    
    // Outro código do programa principal
    
    pthread_exit(NULL);
}
```

## Passo 5
### Compilar e executar o programa
Agora que você concluiu a implementação básica do programa, compile-o usando um compilador C que suporte pthreads. Por exemplo, você pode usar o GCC com a opção -pthread para habilitar o suporte a pthreads:

```bash
gcc -pthread meu_programa.c -o meu_programa
```

### Agora, execute o programa:

```bash
./meu_programa
```

Parabéns! Você criou um programa C simples que utiliza pthreads para criar e executar uma thread, passando parâmetros usando uma struct. Você pode expandir esse exemplo adicionando mais parâmetros à struct e implementando lógica mais complexa dentro da função da thread.

Lembre-se de tomar cuidado ao passar a struct como argumento para a thread, pois ela deve permanecer válida enquanto a thread estiver em execução. Você pode usar alocação dinâmica de memória para evitar problemas de escopo.

Espero que este tutorial seja útil para você começar a usar pacotes (structs) em C com pthreads!