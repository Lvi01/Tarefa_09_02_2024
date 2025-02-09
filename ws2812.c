// Autor: Levi Silva Freitas
// Data: 2025-02-02

// Inclusão de bibliotecas necessárias para o programa
#include <stdio.h>                // Inclusão da biblioteca padrão de entrada e saída
#include <stdlib.h>              // Inclusão da biblioteca padrão do C
#include "pico/stdlib.h"        // Inclusão da biblioteca de funções padrão do Pico
#include "hardware/pio.h"      // Inclusão da biblioteca de funções do PIO
#include "hardware/clocks.h"  // Inclusão da biblioteca de funções de clock
#include "hardware/i2c.h"    // Inclusão da biblioteca de funções do I2C
#include "ws2812.pio.h"     // Inclusão da biblioteca de funções do WS2812B
#include "inc/ssd1306.h"   // Inclusão da biblioteca de funções de display e configuração do OLED
#include "inc/font.h"     // Inclusão da biblioteca de funções de fonte

// Definições de constantes utilizadas no programa
#define IS_RGBW false          // Define se a matriz é RGB ou RGBW
#define NUM_PIXELS 25         // Quantidade de nuúmeros de LEDs na matriz
#define NUMBERS 10           // Quantidade de números que aparecerão na matriz
#define WS2812_PIN 7        // GPIO7 responsável pela comunicação com a matriz de LEDs
#define TEMPO 200          // Tempo de espera em ms, faz com que o LED Vermelho pisque 5 vezes por segundo
#define I2C_PORT i2c1     // Define a porta I2C utilizada
#define I2C_SDA 14       // Define o pino SDA
#define I2C_SCL 15      // Define o pino SCL
#define endereco 0x3C  // Endereço do display OLED

// Pinos para controle do LED e botões
const uint ledRed_pin = 13;     // Red => GPIO13
const uint ledBlue_pin = 12;   // Blue => GPIO12
const uint ledGreen_pin = 11; // Green => GPIO11
const uint button_A = 5;     // Botão A => GPIO5
const uint button_B = 6;    // Botão B => GPIO6

ssd1306_t ssd; // Inicializa a estrutura do display

// Variáveis globais para controle do LED e cor
uint8_t displayed_number = 0;      // Índice do LED a ser controlado (0 a 24)
uint8_t selected_r = 0;           // Intensidade do vermelho (0 a 255)
uint8_t selected_g = 0;          // Intensidade do verde (0 a 255)
uint8_t selected_b = 255;       // Intensidade do azul (0 a 255)

// Variáveis globais para controle do tempo
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// Buffer para armazenar as cores de todos os LEDs
// Cada coluna representa um número de 0 a 9
bool led_buffer[NUMBERS][NUM_PIXELS] = {
    // Número 0
    0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0,

    // Número 1
    0, 1, 1, 1, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 1, 1, 0, 0,
    0, 0, 1, 0, 0,

    // Número 2
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,

    // Número 3
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,

    // Número 4
    0, 1, 0, 0, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 0, 1, 0,

    // Número 5
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0,

    // Número 6
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 0, 0,
    0, 1, 1, 1, 0,

    // Número 7
    0, 0, 0, 1, 0,
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,

    // Número 8
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0,

    // Número 9
    0, 1, 1, 1, 0,
    0, 0, 0, 1, 0,
    0, 1, 1, 1, 0,
    0, 1, 0, 1, 0,
    0, 1, 1, 1, 0
};

// Prototipação das funções utilizadas no programa

/**
 * @brief Inicializa os componentes necessários.
 * 
 * @return true se a inicialização for bem-sucedida, false caso contrário.
 */
bool init_components();                                                            

/**
 * @brief Função de interrupção com debouncing.
 * 
 * @param gpio O pino GPIO que gerou a interrupção.
 * @param events Os eventos que ocorreram.
 */
static void gpio_irq_handler(uint gpio, uint32_t events);                         

/**
 * @brief Envia um pixel para a matriz de LEDs.
 * 
 * @param pixel_grb O valor do pixel em formato GRB.
 */
static inline void put_pixel(uint32_t pixel_grb);                                

/**
 * @brief Converte os valores de RGB para um valor de 32 bits.
 * 
 * @param r Intensidade do vermelho (0 a 255).
 * @param g Intensidade do verde (0 a 255).
 * @param b Intensidade do azul (0 a 255).
 * @return uint32_t O valor de 32 bits representando a cor.
 */
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);               

/**
 * @brief Define o padrão dos LEDs.
 * 
 * @param r Intensidade do vermelho (0 a 255).
 * @param g Intensidade do verde (0 a 255).
 * @param b Intensidade do azul (0 a 255).
 * @param displayed_number O índice da coluna do led_buffer a ser exibida. Faz a seleção do número a ser exibido (0 a 9). 
 */
void set_led_pattern(uint8_t r, uint8_t g, uint8_t b, int displayed_number);

/**
 * @brief Processa o comando recebido.
 * 
 * @param comando O comando recebido.
 */
void processar_comando(char comando);

int main() {
    stdio_init_all(); // Inicializa a comunicação serial

    init_components();                                    // Inicializa os componentes
    if(!init_components()){                              // Verifica se os componentes foram inicializados corretamente
        printf("Erro ao inicializar os componentes\n"); // Caso não sejam, exibe uma mensagem de erro
        return 1;
    }

    char entrada_usuario;  // Buffer para armazenar a entrada do usuário.

    PIO pio = pio0;                                        // Define o PIO utilizado
    int sm = 0;                                           // Define o state machine utilizada
    uint offset = pio_add_program(pio, &ws2812_program); // Adiciona o programa ao PIO

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW); // Inicializa o programa do WS2812B

    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);  // Habilita a interrupção no botão A
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita a interrupção no botão B

    set_led_pattern(selected_r, selected_g, selected_b, displayed_number); // Define o padrão inicial dos LEDs, começando com o número 0

    while (true) {
        if (scanf("%c", &entrada_usuario) != EOF) { // Lê a entrada do usuário
            processar_comando(entrada_usuario);  // Processa o comando digitado.
        }
    }

    return 0;
}

bool init_components(){
    // Inicializa os LEDs vermelho, azul e verde e os botões
    gpio_init(ledRed_pin);                     // Inicializa o pino do LED
    gpio_set_dir(ledRed_pin, GPIO_OUT);       // Configura o pino como saída
    gpio_init(ledBlue_pin);                  // Inicializa o pino do LED
    gpio_set_dir(ledBlue_pin, GPIO_OUT);    // Configura o pino como saída
    gpio_init(ledGreen_pin);               // Inicializa o pino do LED
    gpio_set_dir(ledGreen_pin, GPIO_OUT); // Configura o pino como saída

    gpio_init(button_A);                // Inicializa o botão
    gpio_set_dir(button_A, GPIO_IN);   // Configura o pino como entrada
    gpio_pull_up(button_A);           // Habilita o pull-up interno

    gpio_init(button_B);                // Inicializa o botão
    gpio_set_dir(button_B, GPIO_IN);   // Configura o pino como entrada
    gpio_pull_up(button_B);           // Habilita o pull-up interno

    // I2C inicialização e configuração do display OLED SSD1306 128x64 pixels com endereço 0x3C e 400 KHz
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display OLED
    i2c_init(I2C_PORT, 400 * 1000); // Inicializa o I2C com 400 KHz

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                    // Pull up the data line
    gpio_pull_up(I2C_SCL);                   // Pull up the clock line
    ssd1306_config(&ssd);                   // Configura o display
    ssd1306_send_data(&ssd);               // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd);  // Atualiza o display

    return true;
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b); // Converte os valores de RGB para um valor de 32 bits
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u); // Envia o valor do pixel para o PIO de forma bloqueante, deslocando 8 bits para a esquerda
}

// Função de interrupção com debouncing
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Obtém o tempo atual em microssegundos
    bool state = false; // Estado do LED

    // Verifica se passou tempo suficiente desde o último evento, para evitar o debouncing
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento

        // Verifica qual botão foi pressionado, com base na GPIO de entrada, e atualiza o estado do LED que está relacionado a ele
        switch (gpio){
        case 5:
            state = gpio_get(ledRed_pin); // Obtém o estado do LED vermelho
            printf("Botão A pressionado\n");
            printf("Mudando o estado do LED vermelho\n");
            if(!state) // Verifica se o LED está ligado
            {
                printf("LED vermelho ligado\n");
                ssd1306_fill(&ssd, false);                        // Limpa o display
                ssd1306_draw_string(&ssd, "LED VERMELHO", 0, 0); // Desenha uma string
                ssd1306_draw_string(&ssd, "LIGADO", 0, 20);     // Desenha uma string
            }
            else // Caso o LED esteja desligado
            {
                printf("LED vermelho desligado\n");
                ssd1306_fill(&ssd, false);                        // Limpa o display
                ssd1306_draw_string(&ssd, "LED VERMELHO", 0, 0); // Desenha uma string
                ssd1306_draw_string(&ssd, "DESLIGADO", 0, 20);  // Desenha uma string
            }
            gpio_put(ledRed_pin, !state); // Muda o estado do LED vermelho
            ssd1306_send_data(&ssd); // Atualiza o display
            break;
        case 6:
            state = gpio_get(ledBlue_pin); // Obtém o estado do LED azul
            printf("Botão b pressionado\n");
            printf("Mudando o estado do LED azul\n");
            if(!state) // Verifica se o LED está ligado
            {
                printf("LED azul ligado\n");
                ssd1306_fill(&ssd, false); // Limpa o display
                ssd1306_draw_string(&ssd, "LED AZUL     ", 0, 0); // Desenha uma string
                ssd1306_draw_string(&ssd, "LIGADO", 0, 20); // Desenha uma string
            }
            else // Caso o LED esteja desligado
            {
                printf("LED azul desligado\n");
                ssd1306_fill(&ssd, false);                         // Limpa o display
                ssd1306_draw_string(&ssd, "LED AZUL     ", 0, 0); // Desenha uma string
                ssd1306_draw_string(&ssd, "DESLIGADO", 0, 20);   // Desenha uma string
            }
            gpio_put(ledBlue_pin, !state); // Muda o estado do LED azul
            ssd1306_send_data(&ssd);      // Atualiza o display
            break;
        default:
            break;
        }        
    }
}

void set_led_pattern(uint8_t r, uint8_t g, uint8_t b, int displayed_number)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (led_buffer[displayed_number][i]) // Verifica se o LED deve ser ligado e qual número ele representa
            put_pixel(color);               // Liga o LED com um no buffer
        else
            put_pixel(0);  // Desliga os LEDs com zero no buffer
    }
}

void processar_comando(char comando) {
    if(!(comando >= '0' && comando <= '9' || comando >= 'A' && comando <= 'Z' || comando >= 'a' && comando <= 'z')) // Verifica se o comando é um número ou uma letra
    {
        printf("Char inválido\n");                        // Exibe uma mensagem de erro
        ssd1306_fill(&ssd, false);                       // Limpa o display
        ssd1306_draw_string(&ssd, "ERRO", 0, 0);        // Desenha uma string
        ssd1306_draw_string(&ssd, "CHAR", 0, 20);      // Desenha uma string
        ssd1306_draw_string(&ssd, "INVALIDO", 0, 40); // Desenha uma string
        ssd1306_send_data(&ssd);                     // Atualiza o display
        return;
    }
    printf("Char recebido: %c\n", comando);               // Exibe o comando recebido
    ssd1306_fill(&ssd, false);                           // Limpa o display
    ssd1306_draw_string(&ssd, "CHAR RECEBIDO", 0, 0);   // Desenha uma string
    ssd1306_draw_char(&ssd, comando, 60, 32);          // Desenha um caractere
    ssd1306_send_data(&ssd);                          // Atualiza o display
    if(comando >= '0' && comando <= '9')             // Verifica se o comando é um número
    {
        switch(comando)
        {
            case '0':
                displayed_number = 0; // Define o número 0
                break;
            case '1':
                displayed_number = 1; // Define o número 1
                break;
            case '2':
                displayed_number = 2; // Define o número 2
                break;
            case '3':
                displayed_number = 3; // Define o número 3
                break;
            case '4':
                displayed_number = 4; // Define o número 4
                break;
            case '5':
                displayed_number = 5; // Define o número 5
                break;
            case '6':
                displayed_number = 6; // Define o número 6
                break;
            case '7':
                displayed_number = 7; // Define o número 7
                break;
            case '8':
                displayed_number = 8; // Define o número 8
                break;
            case '9':
                displayed_number = 9; // Define o número 9
                break;
            default:
                break;
        }
        set_led_pattern(selected_r, selected_g, selected_b, displayed_number); // Define o padrão dos LEDs
    }
}