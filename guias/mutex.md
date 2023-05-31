# Mutex em C com Pthreads
Aqui está um tutorial fácil de entender sobre como usar as funções mutex_lock e mutex_unlock em C com pthreads para garantir a exclusão mútua e evitar condições de corrida.

## Passo 1
### Incluir as bibliotecas necessárias
Comece incluindo as bibliotecas necessárias para usar pthreads no seu programa. Você precisará incluir a biblioteca <pthread.h>.


```c
#include <pthread.h>
```

## Passo 2
### Definir uma variável mutex
Agora, você precisa definir uma variável do tipo pthread_mutex_t para representar o mutex. O mutex será usado para proteger uma seção crítica do código que deve ser acessada exclusivamente por uma única thread por vez.

```c
pthread_mutex_t mutex;
```

## Passo 3
### Inicializar o mutex
Dentro da função main(), antes de criar as threads, você precisa inicializar o mutex usando a função pthread_mutex_init(). Essa função recebe o endereço da variável mutex e um ponteiro para atributos (normalmente, pode ser NULL para os atributos padrão).

```c
int main() {
    pthread_mutex_init(&mutex, NULL);
    
    // Resto do código...
}
```

## Passo 4
### Proteger a seção crítica com mutex_lock e mutex_unlock
Agora, você pode usar as funções pthread_mutex_lock() e pthread_mutex_unlock() para proteger a seção crítica do código que precisa ser acessada exclusivamente por uma única thread por vez.

```c
void* minhaThread(void* arg) {
    // Código antes da seção crítica
    
    pthread_mutex_lock(&mutex); // Bloqueia o acesso de outras threads
    
    // Seção crítica - código que deve ser executado exclusivamente por uma thread por vez
    
    pthread_mutex_unlock(&mutex); // Libera o acesso para outras threads
    
    // Código após a seção crítica
    
    pthread_exit(NULL);
}
```

## Passo 5
### Destruir o mutex
Após a finalização do uso do mutex, você deve destruí-lo para liberar os recursos associados usando a função pthread_mutex_destroy().


```c
int main() {
    // Resto do código...
    
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
```

## Passo 6
### Compilar e executar o programa
Agora que você concluiu a implementação básica do programa, compile-o usando um compilador C que suporte pthreads. Por exemplo, você pode usar o GCC com a opção -pthread para habilitar o suporte a pthreads:

```bash
gcc -pthread meu_programa.c -o meu_programa
```

### Agora, execute o programa:

```bash
./meu_programa
```

Parabéns! Você criou um programa C simples que utiliza mutex_lock e mutex_unlock em pthreads para garantir a exclusão mútua e evitar condições de corrida. Você pode expandir esse exemplo adicionando mais seções críticas e implementando lógica mais complexa dentro das threads.

Lembre-se de utilizar o mutex corretamente, garantindo que todas as threads que compartilham a seção crítica chamem mutex_lock antes de acessá-la e mutex_unlock após sua utilização.

Espero que este tutorial seja útil para você começar a usar mutex_lock e mutex_unlock em C com pthreads!