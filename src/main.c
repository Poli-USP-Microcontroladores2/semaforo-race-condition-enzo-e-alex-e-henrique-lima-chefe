/*
 * Exemplo demonstrativo de Race Condition no FRDM-KL25Z (Zephyr)
 *
 * Cenário realista: duas threads (producer-like) precisam contar eventos
 * ou amostras e atualizam um contador compartilhado sem usar mutex/atomics.
 * Ambas executam a operação RMW (read-modify-write) sobre `shared_counter`.
 * Uma terceira thread reporta o resultado comparando:
 *    expected_total = local_count_thread1 + local_count_thread2
 * com
 *    shared_counter
 *
 * Se não houver sincronização, atualizações concorrentes se perdem e
 * shared_counter < expected_total — esse é o sinal clássico de Race Condition.
 *
 * Compile para Zephyr (FRDM-KL25Z) e execute. Observe a discrepância.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <stdint.h>

/* -- parâmetros do teste -- */
#define INC_ITERATIONS 100000   /* número de incrementos por thread (aumente/diminua conforme necessário) */
#define REPORT_PERIOD_MS 1000   /* período para imprimir status (ms) */

/* -- recurso compartilhado (intencionalmente sem proteção) -- */
volatile uint32_t shared_counter = 0;

/* Contadores locais para cada thread (usados como "esperado") */
volatile uint32_t local_count_t1 = 0;
volatile uint32_t local_count_t2 = 0;

/* Flags para indicar término das threads incrementadoras */
volatile bool t1_done = false;
volatile bool t2_done = false;

/* Thread stacks e objetos (tamanhos adequados para FRDM-KL25Z) */
K_THREAD_STACK_DEFINE(stack_t1, 1024);
K_THREAD_STACK_DEFINE(stack_t2, 1024);
K_THREAD_STACK_DEFINE(stack_report, 1024);

static struct k_thread th_t1_data;
static struct k_thread th_t2_data;
static struct k_thread th_report_data;

K_SEM_DEFINE(contador, 1, 1);

/* Thread 1: incrementa o contador compartilhado sem sincronização */
void thread_inc1(void *p1, void *p2, void *p3)
{
    uint32_t i;

    for (i = 0; i < INC_ITERATIONS; ++i) {
        k_sem_take(&contador, K_FOREVER);

        /* read-modify-write não-atomico INTENCIONAL */
        uint32_t tmp = shared_counter;  /* lê valor atual */
        tmp = tmp + 1;                  /* modifica localmente */

        /* força um ponto de yield para aumentar chance de preempção
         * (aumenta a probabilidade de reproduzir a Race Condition) */
        k_yield();

        shared_counter = tmp;           /* escreve de volta - possível perda se outra thread escreveu */

        k_sem_give(&contador);
        
        local_count_t1++;               /* contador local, é apenas informativo */
    }

    t1_done = true;
    printk("Thread 1 terminou.\n");
    return;
}

/* Thread 2: faz idem (simula segunda tarefa concorrente) */
void thread_inc2(void *p1, void *p2, void *p3)
{
    uint32_t i;
    for (i = 0; i < INC_ITERATIONS; ++i) {
        k_sem_take(&contador, K_FOREVER);

        uint32_t tmp = shared_counter;
        tmp = tmp + 1;

        /* aqui colocamos uma breve espera para variar o padrão de preempção */
        if ((i & 0xFF) == 0) {
            k_yield();
        }

        shared_counter = tmp;

        k_sem_give(&contador);
        
        local_count_t2++;
    }

    t2_done = true;
    printk("Thread 2 terminou.\n");
    return;
}

/* Thread report: periodicamente imprime estado observável */
void thread_report(void *p1, void *p2, void *p3)
{
    while (1) {
        k_msleep(REPORT_PERIOD_MS);

        uint32_t expected = local_count_t1 + local_count_t2;
        uint32_t actual = shared_counter;

        printk("Relatório: expected(total) = %u, shared_counter = %u (lost = %u)\n",
               expected, actual, (expected > actual) ? (expected - actual) : 0u);

        /* Se ambas as threads terminaram, imprime resumo final e para */
        if (t1_done && t2_done) {
            printk("=== FINAL ===\n");
            printk("local_count_t1 = %u\n", local_count_t1);
            printk("local_count_t2 = %u\n", local_count_t2);
            printk("expected total = %u\n", expected);
            printk("shared_counter = %u\n", actual);
            printk("Perda (expected - actual) = %u\n", (expected > actual) ? (expected - actual) : 0u);
            while (1) {
                k_msleep(1000);
            }
        }
    }
}

/* main cria as threads com mesma prioridade para maximizar competição */
void main(void)
{
    printk("Demo Correção Race Condition - iniciando\n");
    printk("Cada thread fará %d incrementos.\n", INC_ITERATIONS);

    /* Criar thread 1 e 2 com mesma prioridade (teste de competição) */
    k_thread_create(&th_t1_data, stack_t1, K_THREAD_STACK_SIZEOF(stack_t1),
                    thread_inc1, NULL, NULL, NULL,
                    7, 0, K_NO_WAIT);

    k_thread_create(&th_t2_data, stack_t2, K_THREAD_STACK_SIZEOF(stack_t2),
                    thread_inc2, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    /* Thread de reporte (prioridade menor) */
    k_thread_create(&th_report_data, stack_report, K_THREAD_STACK_SIZEOF(stack_report),
                    thread_report, NULL, NULL, NULL,
                    6, 0, K_NO_WAIT);
}
