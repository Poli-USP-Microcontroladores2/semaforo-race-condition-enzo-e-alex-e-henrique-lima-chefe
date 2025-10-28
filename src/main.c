#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(biscoito_mutex, LOG_LEVEL_INF);
#define BISCOITOS_INICIAIS 10
volatile int biscoitos_no_pote = BISCOITOS_INICIAIS;
int biscoitos_gustavo = 0;
int biscoitos_vitor = 0;
k_tid_t tid_gustavo, tid_vitor;

// Mutex para proteger o pote
struct k_mutex pote_mutex;

void gustavo_come_biscoitos(void *arg1, void *arg2, void *arg3){
    while (1) {
        // Início da região crítica protegida
        k_mutex_lock(&pote_mutex, K_FOREVER);
        if (biscoitos_no_pote > 0) {
            int biscoitos_restantes = biscoitos_no_pote;
            biscoitos_gustavo++;
            biscoitos_no_pote = biscoitos_restantes - 1;
            LOG_INF("GUSTAVO: Peguei um biscoito. Pote tinha: %d, Agora tem: %d",
                    biscoitos_restantes, biscoitos_no_pote);
        } else {
            k_mutex_unlock(&pote_mutex);
           break;
        }
        k_mutex_unlock(&pote_mutex);
        k_msleep(50);
    }
    LOG_WRN("GUSTAVO: Acabaram os biscoitos! Comi %d", biscoitos_gustavo);
}

void vitor_come_biscoitos(void *arg1, void *arg2, void *arg3){
    while (1) {
        k_mutex_lock(&pote_mutex, K_FOREVER);
        if (biscoitos_no_pote > 0) {
            int biscoitos_restantes = biscoitos_no_pote;
            biscoitos_vitor++;
            biscoitos_no_pote = biscoitos_restantes - 1;
            LOG_INF("VITOR: Peguei um biscoito. Pote tinha: %d, Agora tem: %d",
                    biscoitos_restantes, biscoitos_no_pote);
        } else {
            k_mutex_unlock(&pote_mutex);
            break;
        }
        k_mutex_unlock(&pote_mutex);
        k_msleep(50);
    }
    LOG_WRN("VITOR: Acabaram os biscoitos! Comi %d", biscoitos_vitor);
}

#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(gustavo_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(vitor_stack, STACK_SIZE);

static struct k_thread gustavo_thread;
static struct k_thread vitor_thread;

void main(void) {
    LOG_INF("=== INÍCIO DO EXPERIMENTO (MUTEX) ===");

    // Inicializa o mutex
    k_mutex_init(&pote_mutex);

    tid_gustavo = k_thread_create(&gustavo_thread, gustavo_stack, K_THREAD_STACK_SIZEOF(gustavo_stack), gustavo_come_biscoitos, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
    tid_vitor = k_thread_create(&vitor_thread, vitor_stack, K_THREAD_STACK_SIZEOF(vitor_stack), vitor_come_biscoitos, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_thread_join(tid_gustavo, K_FOREVER);
    k_thread_join(tid_vitor, K_FOREVER);

    LOG_INF("=== RESULTADO FINAL ===");
    LOG_INF("Total comido: %d (Gustavo=%d, Vitor=%d), Pote restante=%d", biscoitos_gustavo + biscoitos_vitor, biscoitos_gustavo, biscoitos_vitor, biscoitos_no_pote);
}