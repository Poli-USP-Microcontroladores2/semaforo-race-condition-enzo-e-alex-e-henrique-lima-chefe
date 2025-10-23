#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

// Define o módulo de logging
LOG_MODULE_REGISTER(biscoito_race, LOG_LEVEL_DBG);

// Número inicial de biscoitos (par)
#define BISCOITOS_INICIAIS 10

// Variável compartilhada - número de biscoitos no pote
volatile int biscoitos_no_pote = BISCOITOS_INICIAIS;

// Contadores individuais de biscoitos comidos
int biscoitos_gustavo = 0;
int biscoitos_vitor = 0;

// IDs das threads
k_tid_t tid_gustavo, tid_vitor;

// Função que simula o Gustavo comendo biscoitos
void gustavo_come_biscoitos(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        // REGIÃO CRÍTICA - INÍCIO
        // Verifica se ainda há biscoitos
        if (biscoitos_no_pote > 0) {
            // RACE CONDITION: Aqui ocorre o problema!
            // Ambas as threads podem ler o mesmo valor de biscoitos_no_pote
            // antes de qualquer uma atualizar a variável
            int biscoitos_restantes = biscoitos_no_pote;
            
            // Simula um pequeno delay que pode aumentar a chance da race condition
            k_msleep(10);
            
            // Atualiza o contador pessoal
            biscoitos_gustavo++;
            
            // Atualiza o pote compartilhado
            biscoitos_no_pote = biscoitos_restantes - 1;
            
            LOG_INF("GUSTAVO: Peguei um biscoito. Pote tinha: %d, Agora tem: %d, Meus biscoitos: %d", 
                   biscoitos_restantes, biscoitos_no_pote, biscoitos_gustavo);
        } else {
            // Não há mais biscoitos, sai do loop
            break;
        }
        // REGIÃO CRÍTICA - FIM
        
        // Delay para simular o tempo de comer
        k_msleep(50);
    }
    
    LOG_WRN("GUSTAVO: Acabaram os biscoitos! Comi %d biscoitos", biscoitos_gustavo);
}

// Função que simula o Vitor comendo biscoitos
void vitor_come_biscoitos(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        // REGIÃO CRÍTICA - INÍCIO
        // Verifica se ainda há biscoitos
        if (biscoitos_no_pote > 0) {
            // RACE CONDITION: O mesmo problema ocorre aqui
            int biscoitos_restantes = biscoitos_no_pote;
            
            // Simula um pequeno delay que pode aumentar a chance da race condition
            k_msleep(10);
            
            // Atualiza o contador pessoal
            biscoitos_vitor++;
            
            // Atualiza o pote compartilhado
            biscoitos_no_pote = biscoitos_restantes - 1;
            
            LOG_INF("VITOR: Peguei um biscoito. Pote tinha: %d, Agora tem: %d, Meus biscoitos: %d", 
                   biscoitos_restantes, biscoitos_no_pote, biscoitos_vitor);
        } else {
            // Não há mais biscoitos, sai do loop
            break;
        }
        // REGIÃO CRÍTICA - FIM
        
        // Delay para simular o tempo de comer
        k_msleep(50);
    }
    
    LOG_WRN("VITOR: Acabaram os biscoitos! Comi %d biscoitos", biscoitos_vitor);
}

// Definição das stacks das threads
#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(gustavo_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(vitor_stack, STACK_SIZE);

// Estruturas das threads
static struct k_thread gustavo_thread;
static struct k_thread vitor_thread;

void main(void)
{
    LOG_INF("=== INÍCIO DO EXPERIMENTO RACE CONDITION ===");
    LOG_INF("Pote inicial com %d biscoitos", BISCOITOS_INICIAIS);
    LOG_INF("Gustavo e Vitor começam a comer...");
    
    // Cria as threads dos irmãos
    tid_gustavo = k_thread_create(&gustavo_thread, gustavo_stack,
                                 K_THREAD_STACK_SIZEOF(gustavo_stack),
                                 gustavo_come_biscoitos,
                                 NULL, NULL, NULL,
                                 5, 0, K_NO_WAIT);
    
    tid_vitor = k_thread_create(&vitor_thread, vitor_stack,
                               K_THREAD_STACK_SIZEOF(vitor_stack),
                               vitor_come_biscoitos,
                               NULL, NULL, NULL,
                               5, 0, K_NO_WAIT);
    
    // Aguarda ambas as threads terminarem
    k_thread_join(tid_gustavo, K_FOREVER);
    k_thread_join(tid_vitor, K_FOREVER);
    
    // Resultado final
    LOG_INF("=== RESULTADO FINAL ===");
    LOG_INF("Biscoitos totais comidos: %d", biscoitos_gustavo + biscoitos_vitor);
    LOG_INF("Biscoitos no pote restantes: %d", biscoitos_no_pote);
    LOG_INF("Gustavo comeu: %d biscoitos", biscoitos_gustavo);
    LOG_INF("Vitor comeu: %d biscoitos", biscoitos_vitor);
    
    // Verifica quem paga o próximo pote
    if (biscoitos_gustavo > biscoitos_vitor) {
        LOG_ERR("RESULTADO INJUSTO! Gustavo comeu mais e Vitor deve pagar o próximo pote!");
    } else if (biscoitos_vitor > biscoitos_gustavo) {
        LOG_ERR("RESULTADO INJUSTO! Vitor comeu mais e Gustavo deve pagar o próximo pote!");
    } else {
        LOG_INF("Resultado justo! Ambos comeram a mesma quantidade.");
    }
    
    // Evidência da race condition
    int total_comido = biscoitos_gustavo + biscoitos_vitor;
    int biscoitos_esperados = BISCOITOS_INICIAIS - biscoitos_no_pote;
    
    if (total_comido != BISCOITOS_INICIAIS) {
        LOG_ERR("RACE CONDITION DETECTADA!");
        LOG_ERR("Total comido (%d) != Biscoitos iniciais (%d)", total_comido, BISCOITOS_INICIAIS);
        LOG_ERR("Isso significa que alguns biscoitos foram 'duplicados' devido à race condition!");
    }
}
