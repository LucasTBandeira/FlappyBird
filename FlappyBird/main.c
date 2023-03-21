#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "nokia5110.h"
#include <math.h>

#define TIMER_CLK F_CPU / 1024
#define IRQ_FREQ 32

void init();                                                               // Função para inicializar portas e setar timer
void button();                                                             // Função para lidar com o clique no botão. Troca as telas e estado do passarinho
void desenhaMenu();                                                        // Desenha a tela de MENU no LCD
void desenhaOver();                                                        // Desenha a tela de OVER no LCD, ao jogador morrer;
void desenhaLimiteH();                                                     // Desenha linha horizontal superior, na tela de GAME
void desenhaLimiteV();                                                     // Desenha linha vertical superior, na tela de GAME
void desenhaObstaculo(uint8_t xP, uint8_t yPC, uint8_t yPB, uint8_t pipe); // Desenha obstáculo, considerando as coordenadas e o valor de pipe
void desenhaPassaro(uint8_t y);                                            // Desenha o pássaro, com x constante e y variável
void gameLoop();                                                           // Loop do jogo (LCD e colisões)

uint8_t pontuacao = 0; // Pontuação do jogador
uint8_t pipe = 1;      // Obstáculo
char str[20];

uint8_t taxa_descida = 0; // Quantidade de pixels de descida
uint8_t topo = 0;         // Variável para definir até qual posição o passarinho deve ir, ao pular
uint8_t base = 0;         // Variável para definir até qual posição o passarinho deve ir, se não pular

uint8_t yB = 22;      // Coordenada y do passarinho. O x é constante
uint8_t x, y, x2, y2; // Coordenadas para desenhar linhas horizontais / verticais no LCD

uint8_t xP = 79;  // x inicial do obstáculo
uint8_t yPC = 45; // y da parte de cima do obstáculo
uint8_t yPB = 2;  // y da parte de baixo do obstáculo

enum STATE
{
    UP,   // Passarinho está realizando o pulo
    DOWN, // Passarinho não está realizando o pulo (= terminou um pulo (qtd = topo) e o jogador não iniciou outro)
    IDLE  // Estado inicial do jogo (= Menu)
};

enum SCENE
{
    MENU, // Tela inicial do jogo
    GAME, // Tela do jogo
    OVER  // Tela final do jogo
};

enum STATE estado = IDLE;
enum SCENE cena = MENU;

void init()
{
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = (TIMER_CLK / IRQ_FREQ) - 1;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);

    DDRD &= ~(1 << PD0);
    PORTD &= ~(1 << PD0);

    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT16);
}

void button()
{
    if (PIND & (1 << PD0)) // Movimentação do passarinho
    {
        switch (cena)
        {
        case MENU:
            cena = GAME;
            break;
        case OVER:
            cena = MENU;
            estado = IDLE;
            break;
        case GAME:
            estado = UP;
            topo = yB - 9;    // A cada pulo, topo recebe a posição do passarinho + 7 pixels
            taxa_descida = 0; // Reseta a taxa de descida
            break;
        }
        while (PIND & (1 << PD0))
        {
            _delay_ms(1);
        }
    }
}

ISR(PCINT2_vect)
{
    button();
}

ISR(TIMER1_COMPA_vect)
{
    nokia_lcd_clear();

    if (cena == MENU)
    {
        desenhaMenu();
        pontuacao = 0;
    }
    if (cena == GAME)
    {
        gameLoop();

        if (estado == UP)
        {
            if (yB >= topo)
            {
                yB -= 1;
                xP -= 1;
                if (xP == 4)
                {
                    xP = 80;
                    pontuacao++;
                    pipe = rand() % 3 + 1;
                }
            }
            if (yB == topo)
            {
                estado = DOWN;
                base = yB + 9;
                yB += 1;
            }
        }

        if (estado == DOWN)
        {
            if (yB <= base)
            {
                yB += 1;
                xP -= 1;
                if (xP == 4)
                {
                    xP = 80;
                    pontuacao++;
                    pipe = rand() % 3 + 1;
                }
            }
            base += 9;
        }
    }
    if (cena == OVER)
    {
        desenhaOver();
        taxa_descida = 0;
        topo = 0;
        base = 0;
        yB = 22;
        xP = 79;
    }

    nokia_lcd_render();
}

int main(void)
{
    cli();
    init();
    nokia_lcd_init();
    sei();

    while (1)
    {
    }
}

void desenhaMenu()
{
    desenhaLimiteH();
    desenhaLimiteV();
    nokia_lcd_set_cursor(25, 6);
    nokia_lcd_write_string("CLIQUE", 1);
    nokia_lcd_set_cursor(13, 15);
    nokia_lcd_write_string("PARA JOGAR", 1);
    nokia_lcd_drawcircle(43, 33, 8);     // Corpo
    nokia_lcd_drawcircle(46, 32, 8 / 3); // Olho
    nokia_lcd_drawline(51, 33, 55, 33);  // Bico
}

void desenhaOver()
{
    nokia_lcd_set_cursor(13, 15);
    nokia_lcd_write_string("GAME OVER", 1);
    nokia_lcd_drawcircle(24, 33, 8);     // Corpo
    nokia_lcd_drawcircle(27, 32, 8 / 3); // Olho
    nokia_lcd_drawline(32, 33, 36, 33);  // Bico
    sprintf(str, "%d", pontuacao);
    nokia_lcd_set_cursor(44, 29);
    nokia_lcd_write_string(str, 1);
}

void desenhaLimiteH()
{
    x = 0;
    x2 = 84;
    y = 0;
    y2 = 0;
    nokia_lcd_drawline(x, y, x2, y2);
    x = 0;
    x2 = 84;
    y = 47;
    y2 = 47;
    nokia_lcd_drawline(x, y, x2, y2);
}

void desenhaLimiteV()
{
    x = 0;
    x2 = 0;
    y = 1;
    y2 = 47;
    nokia_lcd_drawline(x, y, x2, y2);
    x = 83;
    x2 = 83;
    y = 1;
    y2 = 47;
    nokia_lcd_drawline(x, y, x2, y2);
}

void desenhaObstaculo(uint8_t xP, uint8_t yPC, uint8_t yPB, uint8_t pipe)
{
    switch (pipe)
    {
    case 1:
        nokia_lcd_drawline(xP, yPC, xP, yPC - 15);
        nokia_lcd_drawline(xP, yPB, xP, yPB + 12);
        break;
    case 2:
        nokia_lcd_drawline(xP, yPB + 15, xP, yPC);
        break;
    case 3:
        nokia_lcd_drawline(xP, yPB, xP, yPC - 15);
        break;
    }
}

void desenhaPassaro(uint8_t yB)
{
    nokia_lcd_drawcircle(6, yB, 2);
}

void gameLoop()
{
    desenhaLimiteH();

    if (yB >= 44 || yB <= 3) // Testa colisão com os limites verticais e horizontais
    {
        cena = OVER;
    }
    switch (pipe) // Testa colisão com cada tipo de obstáculo. X teste colisão horizontal e y para permitir a passagem
    {
    case 1:
        if (xP == 6 && (yB >= yPC - 15 || yB <= yPB + 12)) 
            cena = OVER;
        break;
    case 2:
        if (xP == 6 && (yB >= yPB + 15))
            cena = OVER;
        break;
    case 3:
        if (xP == 6 && (yB <= yPC - 15))
            cena = OVER;
        break;
    }

    desenhaPassaro(yB);
    desenhaObstaculo(xP, yPC, yPB, pipe);
}