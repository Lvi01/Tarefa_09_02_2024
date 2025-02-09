# Projeto: Interface de Comunicação Serial com RP2040

## Autor
**Levi Silva Freitas**

## Data
**09 de fevereiro de 2025**

## Link de entrega
Link: https://youtube.com/shorts/EytLdvitqN0?feature=share

## Descrição do Projeto
Este projeto visa implementar um sistema embarcado utilizando a placa **BitDogLab** com o microcontrolador **RP2040**. O sistema envolve a comunicação serial via **UART e I2C**, controle de **LEDs RGB e WS2812**, interação por **botões físicos** e exibição de informações em um **display OLED SSD1306**.

O código permite:
- Exibir caracteres recebidos via **Serial Monitor** no **display OLED**.
- Controlar uma **matriz 5x5 de LEDs WS2812** para exibir números de 0 a 9.
- Ligar e desligar LEDs RGB ao pressionar os botões físicos.
- Utilizar **interrupções (IRQ)** para detectar eventos de botão com **debouncing** via software.

## Componentes Utilizados
- **Matriz 5x5 de LEDs WS2812** (endereçáveis) – GPIO **7**.
- **LED RGB** – GPIOs **11, 12 e 13** (Red, Green, Blue).
- **Botão A** – GPIO **5**.
- **Botão B** – GPIO **6**.
- **Display OLED SSD1306** via I2C – GPIOs **14 e 15**.

## Estrutura do Código

### 1. Inclusão de Bibliotecas
O código inclui diversas bibliotecas essenciais para a comunicação e controle de hardware:
```c
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "ws2812.pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
```

### 2. Definição de Constantes e Pinos
Foram definidas constantes para **pinos GPIO** e configurações de hardware:
```c
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
```

### 3. Variáveis Globais
Variáveis globais para armazenar o **estado dos LEDs** e **botões**:
```c
uint8_t displayed_number = 0;
uint8_t selected_r = 0, selected_g = 0, selected_b = 255;
static volatile uint32_t last_time = 0;
```

### 4. Inicialização de Componentes
A função `init_components()` configura os **LEDs**, **botões** e o **display**:
```c
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
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT); // Inicializa o display OLED
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
```

### 5. Controle dos LEDs WS2812
Os LEDs são controlados pela função `set_led_pattern()`, que define o padrão da matriz:
```c
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
```

### 6. Tratamento de Interrupções
Os botões **A e B** utilizam interrupções para alternar os LEDs:
```c
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
            state = gpio_get(ledGreen_pin); // Obtém o estado do LED verde
            printf("Botão A pressionado\n");
            printf("Mudando o estado do LED verde\n");
            if(!state) // Verifica se o LED está ligado
            {
                printf("LED verde ligado\n");
                ssd1306_fill(&ssd, false);                        // Limpa o display
                ssd1306_draw_string(&ssd, "LED VERDE", 0, 0); // Desenha uma string
                ssd1306_draw_string(&ssd, "LIGADO", 0, 20);     // Desenha uma string
            }
            else // Caso o LED esteja desligado
            {
                printf("LED verde desligado\n");
                ssd1306_fill(&ssd, false);                        // Limpa o display
                ssd1306_draw_string(&ssd, "LED VERDE", 0, 0); // Desenha uma string
                ssd1306_draw_string(&ssd, "DESLIGADO", 0, 20);  // Desenha uma string
            }
            gpio_put(ledGreen_pin, !state); // Muda o estado do LED verde
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
```

### 7. Processamento de Entrada Serial
A função `processar_comando()` recebe caracteres via **UART**:
```c
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
```

### 8. Função Principal `main()`
```c
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
```

## Requisitos Atendidos
✅ Comunicação Serial **UART** para entrada de caracteres.  
✅ Controle de **LEDs WS2812** com exibição de números.  
✅ Utilização de **interrupções** para botões.  
✅ Implementação de **debouncing**.  
✅ Exibição no **display SSD1306** via **I2C**.  
✅ Código **estruturado e comentado**.

## Como Executar o Projeto
1. **Compilar o código** para a plataforma **RP2040**.
2. **Carregar o binário** na placa **BitDogLab**.
3. **Abrir o Serial Monitor** no VS Code e enviar caracteres.
4. **Observar a exibição no display OLED e a matriz de LEDs**.

## Conclusão
Este projeto demonstra o uso de **UART, I2C, LEDs e interrupções** em um **RP2040**. O código é modular, organizado e segue as melhores práticas de programação para microcontroladores.

## Contato
✉️ **Email:** lsfreitas218@gmail.com

