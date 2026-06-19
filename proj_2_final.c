//Arthur Alves da Costa - 13751207

// Conexões do Módulo LCD

sbit LCD_RS at LATB4_bit;
sbit LCD_EN at LATB5_bit;
sbit LCD_D4 at LATB0_bit;
sbit LCD_D5 at LATB1_bit;
sbit LCD_D6 at LATB2_bit;
sbit LCD_D7 at LATB3_bit;

sbit LCD_RS_Direction at TRISB4_bit;
sbit LCD_EN_Direction at TRISB5_bit;
sbit LCD_D4_Direction at TRISB0_bit;
sbit LCD_D5_Direction at TRISB1_bit;
sbit LCD_D6_Direction at TRISB2_bit;
sbit LCD_D7_Direction at TRISB3_bit;

// Variáveis para os timers

short counter = 0;            // Armazena o tempo atual no display
unsigned short aux_timer = 0; // Conta as frações de 250ms do Timer 1
unsigned short mode = 0;      // Modo 0 para contagem corta e modo 1 para contagem longa

bit button_flag_1; // Flag auxiliar do botão 1 (RD0)
bit button_flag_2; // Flag auxiliar do botão 2 (RD1)
bit led_warning;

// Variáveis para a Temperatura

unsigned int adc_val = 0;
unsigned long temp_long = 0;
unsigned int temp_x10 = 0;
unsigned char temp_deg = 0;
unsigned char temp_dec = 0;

void main() {
    // Configura AN0 como analógico, o resto como digital

    ADCON1 |= 0x0E;
    CMCON  |= 7;

    // Configura o pino RA0 como entrada para o LM35

    TRISA0_bit = 1;
    TRISA1_bit = 0;
    LATA1_bit = 0;

    // Configuração inicial do Timer 0

    T0CON = 0B00000110;
    T0CON.TMR0ON = 0;
    TMR0H = 0xC2;
    TMR0L = 0xF7;
    INTCON.TMR0IF = 0;

    // Configuração inicial do Timer 1

    T1CON = 0b00110000;
    T1CON.TMR1ON = 0;
    TMR1H = 0x0B;
    TMR1L = 0xDC;
    PIR1.TMR1IF = 0;

    // Configura os pinos RD0 e RD1 (PORTD) como entrada para os botões

    TRISD0_bit = 1;
    TRISD1_bit = 1;

    // Inicialização do LCD no modo 4 bits

    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);       // Limpa o display
    Lcd_Cmd(_LCD_CURSOR_OFF);  // Desliga o cursor

    // Inicialização do ADC

    ADC_Init();

    // Escrita estática da primeira linha

    Lcd_Out(1, 1, "Time   Temp");
    Lcd_Chr(2, 1, '0');
    Lcd_Chr(2, 2, ' ');

    button_flag_1 = 0;
    button_flag_2 = 0;
    led_warning = 0;

    while(1) {

        // Leitura e calculo do LM35

        adc_val = ADC_Read(0); // Le o canal AN0 (RA0)

        // Converte o valor para temperatura multiplicada por 10

        temp_long = ((unsigned long)adc_val * 500 * 10) / 1023;
        temp_x10 = (unsigned int)temp_long;

        // Limita a faixa de 0 a 100 

        if (temp_x10 > 1000) temp_x10 = 1000;

        temp_deg = temp_x10 / 10; // Parte inteira
        temp_dec = temp_x10 % 10; // Parte decimal

        if (temp_x10 > 500){
            led_warning = 1;
        } else {
            led_warning = 0;
        }

        // Lógica do Botão 2 (Pino RD0) - 10 a 0 seg

        if (RD0_bit == 1) {
            Delay_ms(15);
            if (RD0_bit == 1) {
                if (button_flag_1 == 0) {
                    button_flag_1 = 1;

                    mode = 0;
                    counter = 10;     // Inicia de 10

                    T1CON.TMR1ON = 0; // Para o timer de 60s se estiver rodando
                    T0CON.TMR0ON = 1; // Inicia o timer de 10s
                    TMR0H = 0xC2;
                    TMR0L = 0xF7;
                    INTCON.TMR0IF = 0;
                }
            }
        } else {
            button_flag_1 = 0;
        }

        // Lógica do Botão 1 (Pino RD1) - 60 a 0 seg

        if (RD1_bit == 1) {
            Delay_ms(15);
            if (RD1_bit == 1) {
                if (button_flag_2 == 0) {
                    button_flag_2 = 1;

                    mode = 1;
                    counter = 60;      // Inicia de 60
                    aux_timer = 0;

                    T0CON.TMR0ON = 0;  // Para o timer de 10s se estiver rodando
                    T1CON.TMR1ON = 1;  // Inicia o timer de 60s
                    TMR1H = 0x0B;
                    TMR1L = 0xDC;
                    PIR1.TMR1IF = 0;
                }
            }
        } else {
            button_flag_2 = 0;
        }

        // Estouro do Timer 0 (Contagem de 10s)

        if (INTCON.TMR0IF == 1){
            INTCON.TMR0IF = 0;
            TMR0H = 0xC2;
            TMR0L = 0xF7;

            if (mode == 0 && counter > 0) {
                counter--;
                if(counter == 0) T0CON.TMR0ON = 0;
            }
        }

        // Estouro do Timer 1 (Contagem de 60s)

        if (PIR1.TMR1IF == 1){
            PIR1.TMR1IF = 0;
            TMR1H = 0x0B;
            TMR1L = 0xDC;

            aux_timer++;

            if (aux_timer >= 4) { // Deu 1 segundo (4 x 250ms)
                aux_timer = 0;
                if (mode == 1 && counter > 0) {
                    counter--;
                    if(counter == 0) T1CON.TMR1ON = 0;
                }
            }
        }

        // Primeira linha: dados de tempo

        if (counter >= 10) {
            Lcd_Chr(2, 1, (counter / 10) + '0'); // Casa da dezena
            Lcd_Chr(2, 2, (counter % 10) + '0'); // Casa da unidade
        } else {
            Lcd_Chr(2, 1, (counter % 10) + '0'); // Casa da unidade na primeira posição
            Lcd_Chr(2, 2, ' ');                  // Limpa a segunda posição
        }

        // Segunda linha: dados de temperatura se o contador estiver ativo

        if (counter > 0) {
            if (temp_deg >= 100) {
                Lcd_Chr(2, 8, '1');
                Lcd_Chr(2, 9, '0');
                Lcd_Chr(2, 10, '0');
            } else if (temp_deg >= 10) {
                Lcd_Chr(2, 8, ' ');
                Lcd_Chr(2, 9, (temp_deg / 10) + '0');
                Lcd_Chr(2, 10, (temp_deg % 10) + '0');
            } else {
                Lcd_Chr(2, 8, ' ');
                Lcd_Chr(2, 9, ' ');
                Lcd_Chr(2, 10, temp_deg + '0');
            }

            Lcd_Chr(2, 11, '.');
            Lcd_Chr(2, 12, temp_dec + '0');
            Lcd_Chr(2, 13, 'C');
        } else {

            // Se a contagem parou ou chegou a zero, limpa a área da temperatura

            Lcd_Chr(2, 8, ' ');
            Lcd_Chr(2, 9, ' ');
            Lcd_Chr(2, 10, ' ');
            Lcd_Chr(2, 11, ' ');
            Lcd_Chr(2, 12, ' ');
            Lcd_Chr(2, 13, ' ');
        }

        LATA1_bit = led_warning;

        Delay_ms(50);
    }
}