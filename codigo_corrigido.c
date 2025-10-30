#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>

/* --- Configuração da Simulação --- */
#define INCREMENTS_PER_THREAD 100000 // Pode diminuir para 10000 para ser mais rápido

K_MUTEX_DEFINE(counter_mutex);

/* --- Configuração do Hardware (LEDs via DeviceTree) --- */
#define LED_RED_NODE    DT_ALIAS(led2)
#define LED_GREEN_NODE  DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED_RED_NODE, okay) || !DT_NODE_HAS_STATUS(LED_GREEN_NODE, okay)
#error "O overlay do DeviceTree para os LEDs não foi encontrado"
#endif

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED_RED_NODE, gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);

/* --- Recurso Compartilhado --- */
volatile int shared_counter = 0;

/* --- Definição das Threads --- */
#define STACKSIZE 1024
#define PRIORITY 7

K_THREAD_STACK_DEFINE(thread1_stack_area, STACKSIZE);
K_THREAD_STACK_DEFINE(thread2_stack_area, STACKSIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

/* --- Tarefa que as Threads Executarão (MODIFICADA PARA GARANTIR A FALHA) --- */
void increment_task(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < INCREMENTS_PER_THREAD; ++i) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        // --- SEÇÃO CRÍTICA VULNERÁVEL ---

        // 1. Uma thread lê o valor compartilhado para uma cópia local.
        int local_copy = shared_counter;

        // 2. FORÇAMOS a troca de contexto!
        // A thread atual "dorme" e o escalonador ACORDA a outra thread.
        // A outra thread vai executar o passo 1 (ler o MESMO valor antigo),
        // antes que esta thread possa escrever seu novo valor.
        k_yield();

        // 3. A thread (quando for acordada de novo) incrementa sua cópia local.
        local_copy++;

        // 4. Ela finalmente escreve o valor de volta, mas o incremento da
        //    outra thread será perdido, pois ela também leu o valor antigo.
        shared_counter = local_copy;
        k_mutex_unlock(&counter_mutex);
    }
}


/* --- Função Principal --- */
int main(void)
{
    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&led_red, 0);
    gpio_pin_set_dt(&led_green, 0);

    printf("--- Simulacao de Condicao de Corrida com Zephyr RTOS ---\n");
    printf("Duas threads irao incrementar um contador %d vezes cada.\n", INCREMENTS_PER_THREAD);

    k_thread_create(&thread1_data, thread1_stack_area,
                    K_THREAD_STACK_SIZEOF(thread1_stack_area),
                    increment_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&thread2_data, thread2_stack_area,
                    K_THREAD_STACK_SIZEOF(thread2_stack_area),
                    increment_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_join(&thread1_data, K_FOREVER);
    k_thread_join(&thread2_data, K_FOREVER);

    printf("Threads terminaram.\n");

    int expected_value = INCREMENTS_PER_THREAD * 2;
    printf("-----------------------------------------\n");
    printf("Valor Esperado: %d\n", expected_value);
    printf("Valor Final Real: %d\n", shared_counter);
    printf("-----------------------------------------\n");

    if (shared_counter == expected_value) {
        printf("SUCESSO! Resultado correto (evento raro!).\nAcendendo LED VERDE.\n");
        gpio_pin_set_dt(&led_green, 1);
    } else {
        printf("FALHA! Condicao de corrida detectada!\nAcendendo LED VERMELHO.\n");
        gpio_pin_set_dt(&led_red, 1);
    }

    return 0;
}