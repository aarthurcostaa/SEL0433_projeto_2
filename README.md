# Temporizador de Duplo Modo com Monitoramento de Temperatura (PIC18F4550)

> Projeto de sistemas embarcados utilizando microcontrolador PIC18F4550 para controle de temporização dupla (10s e 60s) via interrupções externas, com monitoramento de temperatura em tempo real via sensor LM35 e exibição em display LCD 16x2.

## Funcionalidades:

  1. Temporizador de 10 segundos: Ativado pelo botão 1 (INT0);
  2. Temporizador de 60 segundos: Ativado pelo botão 2 (INT1);
  3. Monitoramento de Temperatura: Leitura do sensor LM35 via ADC;
  4. LED indicador de segurança ativa caso a temperatura caia abaixo de 60°C e desativa acima de 80°C;
  5. Display LCD 16x2 mostrando as medidas de tempo e temperatura.

## Dimensionamento dos Timers:

Utilizando a frequência interna de 8Mhz e tempo de instrução (Ti) de 0.5μs:

1. Timer 0 (Modo 10 segundos)

    - Configuração (T0CON = 0x06): Modo 16 bits, clock interno, prescaler de 1:128;
   
    - Valor de Preload (0xC2F7): Equivalente a 49911 em decimal.

    - A equação do estouro do Timer 0 é dada por:

        T(overflow​) = (65536 − Preload) × Prescaler × T(instrução)

        Substituindo os valores:

        Toverflow​ = (65536 − 49911) × 128 × 0.5 μs
        Toverflow​ = 15625 × 128 × 0.5 × 10⁻⁶   = 1.0 segundo

        O Timer 0 gera uma interrupção a cada 1 segundo. Portanto, o counter decrementa de 1 em 1 diretamente a cada estouro.

2. Timer 1 (Modo 60 segundos)

    - Configuração (T1CON = 0x30): Modo 16 bits, clock interno, prescaler de 1:8;
    
    - Valor de Preload (0x0BDC): Equivalente a 3036 em decimal.

    - A equação do estouro do Timer 1 é dada por:

        T(overflow​) = (65536 − Preload) × Prescaler × T(instrução)

        Substituindo os valores:

        Toverflow ​= (65536 − 3036) × 8 × 0.5 μs
        Toverflow​ = 62500 × 8 × 0.5 × 10⁻⁶ = 0.25 segundos (250 ms)

        Como o Timer 1 estoura a cada 250 ms, foi implementada uma extensão de contagem via software (aux_timer). O contador principal de segundos só decrementa                       quando o timer estoura 4 vezes:

        4 × 0.25s = 1.0 segundo

## Lógica de Implementação das Interrupções

O sistema baseia-se em uma Máquina de Estados controlada inteiramente por hardware dentro da função void interrupt(). Isso garante que o laço principal (while(1)) não seja sobrecarregado com atrasos (delays) impeditivos.

- Botão INT0:
  Configura Modo = 0;
  Counter = 10s;
  Desliga Timer 1;
  Liga Timer 0 (Base 1s).
  
- Botão INT1:
  Configura Modo = 1;
  Counter = 60s;
  Desliga Timer 0;
  Liga Timer 1 (Base 250ms).

## Vetores de Interrupção Tratados:

  - Interrupções Externas (INT0IF e INT1IF):

     - Funcionam como gatilhos assíncronos de inicialização (Botões).

     - Ao pressionar o Botão 1, a interrupção limpa o flag, define o counter = 10, desliga forçadamente o Timer 1 (evitando conflitos de concorrência) e liga o Timer 0.

     - Ao pressionar o Botão 2, faz o inverso: define counter = 60, zera a variável auxiliar aux_timer e liga o Timer 1.

     - Estouro do Timer 0 (TMR0IF):

       - Recarrega o valor de inicialização (0xC2F7) para manter a precisão do próximo segundo.

       - Decrementa a variável counter. Quando counter == 0, o bit TMR0ON é zerado, desligando o periférico por completo.

     - Estouro do Timer 1 (TMR1IF):

      - Recarrega o valor de inicialização (0x0BDC).

      - Incrementa a variável aux_timer. Ao atingir 4 (completando 1 segundo real), decrementa o counter principal. Ao zerar o tempo, o bit TMR1ON desliga o periférico.

## Especificações de Hardware e Conexões

Mapeamento do LCD (Modo 4 bits):

  - RS:	LATB4	-> (Seleção de Registrador);
  - EN:	LATB5	-> (Enable);
  - D4:	LATD0	-> (Linha de Dados 4);
  - D5:	LATD1	-> (Linha de Dados 5);
  - D6:	LATD2	-> (Linha de Dados 6);
  - D7:	LATD3	-> (Linha de Dados 7).

Periféricos Adicionais:

  - AN0 (RA0): Entrada analógica conectada ao sensor de temperatura LM35;
  - RA1: Saída digital conectada ao LED de Alerta;
  - RB0 (INT0): Botão de disparo do Timer de 10 segundos;
  - RB1 (INT1): Botão de disparo do Timer de 60 segundos.

## Resultados:

Após a compilação do código "proj_2_final.c", indexado acima, temos o seguinte resultado no MikroC Pro for PIC: 

<img width="996" height="147" alt="proj_2_final_1" src="https://github.com/user-attachments/assets/cbea21b1-5368-4d26-ac08-cd9acfd3954c" />

Com a compilação concluída com sucesso, o circuito completo pode ser testado com o seguinte circuito no SimulIde: 

> (Nota-se que para a simulação do sensor LM35 foi utilizado um potencômetro que permite a variação da tensão fornecida à porta A0 do PIC8F4550)

<img width="907" height="632" alt="proj2_simul" src="https://github.com/user-attachments/assets/59d686be-0429-486a-b009-5bf2ae2d069d" />


Finalmente, para retificar o sucesso do projeto, está anexado um vídeo do circuito sendo testado no ambiente de simulação:

https://github.com/user-attachments/assets/a4baf1f5-f64c-467e-9d28-7667269e4e23

