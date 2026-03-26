#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"

const int BOTAO= 22;
const int CHAVE=28;
const int LEDS[5]={2,3,4,5,6};

volatile int botao_flag=0; //1 quando apertar o botao
volatile int g_sw_state = 0; // * Começa em 0 pois SW começa em nível baixo (pull-down)
//1 quando apertar mudar a chave, 0=incrementa

/* ---------------------------------------------------------------
 * Callback de interrupção — chamado pelo hardware quando:
 *   - BTN_PIN tem borda de descida (botão pressionado)
 *   - SW_PIN  tem borda de descida ou subida (chave mudou)
 * Só seta flags, NUNCA faz processamento aqui!
 * --------------------------------------------------------------- */
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BOTAO && events == 0x4) {  // fall = botão pressionado
        botao_flag = 1;
    } else if (gpio == CHAVE) {
        if (events == 0x4) {        // fall = SW foi para nível baixo
            g_sw_state = 0;         // incrementa
        } else if (events == 0x8) { // rise = SW foi para nível alto
            g_sw_state = 1;         // decrementa
        }
    }
}
/* ---------------------------------------------------------------
 * bar_init: inicializa os 5 pinos da barra de LED como saída
 * e garante que todos começam apagados
 * --------------------------------------------------------------- */
void bar_init(void) {
    for (int i = 0; i < 5; i++) {
        gpio_init(LEDS[i]);
        gpio_set_dir(LEDS[i], GPIO_OUT);
        gpio_put(LEDS[i], 0);
    }
}

/* ---------------------------------------------------------------
 * bar_display: recebe valor de 0 a 5 e acende os LEDs
 * correspondentes na barra.
 * Exemplo: val=3 → acende LEDs 0,1,2 e apaga 3,4
 * --------------------------------------------------------------- */
void bar_display(int val) {
    /* Limita o valor entre 0 e 5 para não sair dos limites */
    if (val < 0) val = 0;
    if (val > 5) val = 5;

    for (int i = 0; i < 5; i++) {
        /* Acende se o índice é menor que val, apaga caso contrário */
        if (i < val) {
            gpio_put(LEDS[i], 1); // acende
        } else {
            gpio_put(LEDS[i], 0); // apaga
        }
    }
}


int main() {
    stdio_init_all();

    /* Inicializa barra de LEDs */
    bar_init();

    /* Configura botão com pull-up (nível alto = solto, nível baixo = pressionado) */
    gpio_init(BOTAO);
    gpio_set_dir(BOTAO, GPIO_IN);
    gpio_pull_up(BOTAO);

    /* Configura chave SW sem pull — ela já tem nível definido externamente
     * Monitoramos subida E descida para saber o estado atual sem gpio_get() */
    gpio_init(CHAVE);
    gpio_set_dir(CHAVE, GPIO_IN);
    gpio_pull_up(CHAVE);

    /* Registra callback para o botão (só borda de descida) */
    gpio_set_irq_enabled_with_callback(BOTAO, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    /* Registra callback para a chave (subida E descida para capturar mudanças) */
    gpio_set_irq_enabled(CHAVE, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    /* Valor atual da barra, começa em 0 (todos apagados) */
    int val = 0;

    if (gpio_get(CHAVE)) {
        g_sw_state = 1; // já começa em nível alto = decrementa
    } else {
        g_sw_state = 0;
    }

    bar_display(val);//qualquer valor != 0 é true em C

    while (true) { //qualquer valor != 0 é true em C
        /* Verifica se botão foi pressionado */
        if (botao_flag) {
            botao_flag = 0; // limpa a flag SEMPRE antes de processar

            if (g_sw_state == 0) {
                /* SW em nível baixo → incrementa, limite máximo 5 */
                if (val < 5) val++;
            } else {
                /* SW em nível alto → decrementa, limite mínimo 0 */
                if (val > 0) val--;
            }

            bar_display(val);
            printf("val: %d | sw: %d\n", val, g_sw_state);
        }
    }
}
