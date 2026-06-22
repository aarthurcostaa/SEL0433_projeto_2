// Arthur Alves da Costa - 13751207

// Módulo LCD

sbit LCD_RS at LATB4_bit;
sbit LCD_EN at LATB5_bit;
sbit LCD_D4 at LATD0_bit;
sbit LCD_D5 at LATD1_bit;
sbit LCD_D6 at LATD2_bit;
sbit LCD_D7 at LATD3_bit;

sbit LCD_RS_Direction at TRISB4_bit;
sbit LCD_EN_Direction at TRISB5_bit;
sbit LCD_D4_Direction at TRISD0_bit;
sbit LCD_D5_Direction at TRISD1_bit;
sbit LCD_D6_Direction at TRISD2_bit;
sbit LCD_D7_Direction at TRISD3_bit;

volatile short counter = 0; // Conta quantas vezes o timer extrapolou a contagem
volatile unsigned short aux_timer = 0; // Extensão de contagem para o timer de 60s
volatile unsigned short mode = 0; // Flag de controle para decidir qual timer está sendo usado
volatile unsigned short timer_started = 0; // Controla se algum timer já foi iniciado alguma vez

bit led_warning; // Variável que controla o LED
unsigned int adc_val = 0; // Leitura bruta do conversor
unsigned long temp_long = 0; // Leitura ajustada à referência e resolução
unsigned int temp_x10 = 0; // Temperatura inteira
unsigned char temp_deg = 0; // Parte decimal da leitura
unsigned char temp_dec = 0; // Parte inteira da leitura

void interrupt() {

    // Botão 1 (Timer 10s) - INT0 (RB0)
    if (INT0IF_bit) {
        INT0IF_bit = 0;
        mode = 0;
        counter = 10;
        timer_started = 1;    // Sinaliza que o sistema entrou em atividade

        T1CON.TMR1ON = 0;     // Para o outro processo imediatamente
        T0CON.TMR0ON = 1;     // Liga o timer atual
        TMR0H = 0xC2;
        TMR0L = 0xF7;
    }

    // Botão 2 (Timer 60s) - INT1 (RB1)
    if (INT1IF_bit) {
        INT1IF_bit = 0;
        mode = 1;
        counter = 60;
        aux_timer = 0;
        timer_started = 1;    // Sinaliza que o sistema entrou em atividade

        T0CON.TMR0ON = 0;     // Para o outro processo imediatamente
        T1CON.TMR1ON = 1;     // Liga o timer atual
        TMR1H = 0x0B;
        TMR1L = 0xDC;
    }

    // Estouro do Timer 0 (10s)
    if (TMR0IF_bit) {
        TMR0IF_bit = 0;
        TMR0H = 0xC2;
        TMR0L = 0xF7;

        if (mode == 0 && counter > 0) {
            counter--;
            if(counter == 0) {
                T0CON.TMR0ON = 0;
            }
        }
    }

    // Estouro do Timer 1 (60s)
    if (TMR1IF_bit) {
        TMR1IF_bit = 0;
        TMR1H = 0x0B;
        TMR1L = 0xDC;

        aux_timer++;
        if (aux_timer >= 4) {
            aux_timer = 0;
            if (mode == 1 && counter > 0) {
                counter--;
                if(counter == 0) {
                    T1CON.TMR1ON = 0;
                }
            }
        }
    }
}

void main() {
    
    // Inicializa o módulo ADC primeiro
    
    ADC_Init();

    // Configuração do ADCON1 após a inicialização do ADC
    
    ADCON1 = 0x3B;
    CMCON  |= 7;    // Desliga comparadores analógicos

    // Configuração de direção dos pinos
    
    TRISA0_bit = 1; // AN0 como entrada (LM35)
    TRISA1_bit = 0; // LED como saída
    TRISA2_bit = 1; // AN2 (Vref-) como entrada
    TRISA3_bit = 1; // AN3 (Vref+) como entrada

    LATA1_bit = 0;  // Apaga o LED inicialmente

    // Configura RB0 e RB1 como entradas para os botões
    
    TRISB0_bit = 1;
    TRISB1_bit = 1;

    // Configuração inicial do Timer 0
    
    T0CON = 0B00000110;
    T0CON.TMR0ON = 0;
    TMR0H = 0xC2;
    TMR0L = 0xF7;

    // Configuração inicial do Timer 1
    
    T1CON = 0b00110000;
    T1CON.TMR1ON = 0;
    TMR1H = 0x0B;
    TMR1L = 0xDC;

    // Limpando as flags antes de ligar as chaves das interrupções
    
    TMR0IF_bit = 0;
    TMR1IF_bit = 0;
    INT0IF_bit = 0;
    INT1IF_bit = 0;

    // Configuração da borda das interrupções externas
    
    INTCON2.INTEDG0 = 1;
    INTCON2.INTEDG1 = 1;

    // Habilita as interrupções específicas
    
    INTCON.INT0IE = 1;    // Habilita INT0 (RB0)
    INTCON3.INT1IE = 1;   // Habilita INT1 (RB1)
    INTCON.TMR0IE = 1;    // Habilita Timer 0
    PIE1.TMR1IE = 1;      // Habilita Timer 1

    INTCON.PEIE = 1;      // Habilita periféricos
    INTCON.GIE = 1;       // Liga a chave geral das interrupções

    // Inicializa o LCD

    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Cmd(_LCD_CURSOR_OFF);

    // Escreve o cabeçalho estático.

    Lcd_Out(1, 1, "Time   Temp");

    led_warning = 0;
    timer_started = 0;

while(1) {

    // A lógica roda se o timer estiver ativo ou se ele acabou de zerar, para limpar a tela
    if (timer_started == 1) {

        if (counter > 0) {

            adc_val = ADC_Get_Sample(0);

            temp_long = ((unsigned long)adc_val * 500 * 10) / 1023;
            temp_x10 = (unsigned int)temp_long;

            if (temp_x10 > 1000) temp_x10 = 1000;

            temp_deg = temp_x10 / 10;
            temp_dec = temp_x10 % 10;

            if (temp_x10 < 600) led_warning = 1;
            if (temp_x10 > 800) led_warning = 0;

            // Atualização da temperatura no LCD

            if (temp_deg >= 100) {
                Lcd_Chr(2, 8, '1'); Lcd_Chr(2, 9, '0'); Lcd_Chr(2, 10, '0');
            } else if (temp_deg >= 10) {
                Lcd_Chr(2, 8, ' '); Lcd_Chr(2, 9, (temp_deg / 10) + '0'); Lcd_Chr(2, 10, (temp_deg % 10) + '0');
            } else {
                Lcd_Chr(2, 8, ' '); Lcd_Chr(2, 9, ' '); Lcd_Chr(2, 10, temp_deg + '0');
            }
            Lcd_Chr(2, 11, '.');
            Lcd_Chr(2, 12, temp_dec + '0');
            Lcd_Chr(2, 13, 'C');

            // Atualização do Tempo no LCD

            if (counter >= 10) {
                Lcd_Chr(2, 1, (counter / 10) + '0');
                Lcd_Chr(2, 2, (counter % 10) + '0');
            } else {
                Lcd_Chr(2, 1, (counter % 10) + '0');
                Lcd_Chr(2, 2, ' ');
            }

        } else {

            // Quando o timer chega a 0 limpamos a tela

            Lcd_Chr(2, 1 , '0');
            Lcd_Chr(2, 2 , ' ');
            Lcd_Chr(2, 8 , ' ');
            Lcd_Chr(2, 9 , ' ');
            Lcd_Chr(2, 10, ' ');
            Lcd_Chr(2, 11, ' ');
            Lcd_Chr(2, 12, ' ');
            Lcd_Chr(2, 13, ' ');

            led_warning = 0;
            timer_started = 0;
        }
        LATA1_bit = led_warning;
    }
    Delay_ms(50);
    }
}