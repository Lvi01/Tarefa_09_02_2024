# Projeto: Interface de Comunicação Serial com RP2040

## Autor
**Levi Silva Freitas**

## Data
**02 de fevereiro de 2025**

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
#define endereco 0x3C
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
bool init_components() {
    gpio_init(ledRed_pin);
    gpio_set_dir(ledRed_pin, GPIO_OUT);
    gpio_pull_up(button_A);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    i2c_init(I2C_PORT, 400 * 1000);
    return true;
}
```

### 5. Controle dos LEDs WS2812
Os LEDs são controlados pela função `set_led_pattern()`, que define o padrão da matriz:
```c
void set_led_pattern(uint8_t r, uint8_t g, uint8_t b, int displayed_number) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (led_buffer[displayed_number][i])
            put_pixel(color);
        else
            put_pixel(0);
    }
}
```

### 6. Tratamento de Interrupções
Os botões **A e B** utilizam interrupções para alternar os LEDs:
```c
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (current_time - last_time > 200000) {
        last_time = current_time;
        if (gpio == button_A) {
            gpio_put(ledGreen_pin, !gpio_get(ledGreen_pin));
            printf("LED VERDE alterado\n");
        } else if (gpio == button_B) {
            gpio_put(ledBlue_pin, !gpio_get(ledBlue_pin));
            printf("LED AZUL alterado\n");
        }
    }
}
```

### 7. Processamento de Entrada Serial
A função `processar_comando()` recebe caracteres via **UART**:
```c
void processar_comando(char comando) {
    if (comando >= '0' && comando <= '9') {
        displayed_number = comando - '0';
        set_led_pattern(selected_r, selected_g, selected_b, displayed_number);
    }
    ssd1306_draw_char(&ssd, comando, 60, 32);
    ssd1306_send_data(&ssd);
}
```

### 8. Função Principal `main()`
```c
int main() {
    stdio_init_all();
    init_components();
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    while (true) {
        if (scanf("%c", &entrada_usuario) != EOF) {
            processar_comando(entrada_usuario);
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

