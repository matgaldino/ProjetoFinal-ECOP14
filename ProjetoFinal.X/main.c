//MATHEUS SISTON GALDINO
//2021006340
//PROJETO FINAL ECOP04/ECOP14 - UNIFEI - FORNO ELETRICO DIGITAL
#include<pic18f4520.h>
#include"adc.h"
#include"bits.h"
#include"config.h"
#include"ds1307.h"
#include"keypad.h"
#include"lcd.h"
#include"ssd.h"
#include"timer.h"
#include"pwm.h"
#include"delay.h"

//define comandos para o LCD
#define L_ON 0x0F
#define L_OFF 0X08
#define L_CLR 0x01
#define L_L1 0x80
#define L_L2 0xC0

char modo = 0; //Controle do modo do forno
char state = 0; //Controle se forno ON/OFF
int min = 0, seg = 0; //Controla variaveis do timer
char resist = 0; //Controle da resistencia no modo manual
char start = 0; //Controle timer ON/OFF
unsigned long pot; //Variavel potenciometro
char fim = 0; //Controla o fim do timer

void iniSimbolos(void){ //FUNCAO PARA COLOCAR OS SIMBOLOS ESPECIAIS NA MEMORIA
    char i;
    //simbolos especiais para o LCD
    char cima[8] = {0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x04};
    char baixo[8] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04};
    char cimaBaixo[8] = {0x04, 0x0E, 0x15, 0x04, 0x04, 0x15, 0x0E, 0x04};
    
    //COLOCA O SIMBOLO CIMA NA MEMORIA - lcdChar(0)
    lcdCommand(0x40);
    for(i=0; i<8; i++){
        lcdChar(cima[i]);
    }
    //COLOCA O SIMBOLO BAIXO NA MEMORIA - lcdChar(1)
    lcdCommand(0x48);
    for(i=0; i<8; i++){
        lcdChar(baixo[i]);
    }
    //COLOCA O SIMBOLO CIMA-BAIXO NA MEMORIA - lcdChar(2)
    lcdCommand(0x50);
    for(i=0; i<8; i++){
        lcdChar(cimaBaixo[i]);
    }
}

void desligado(void){ //FUNCAO QUE MOSTRA A DATA E HORA NO LCD
    //Data
    lcdCommand(L_L1);
    lcdString("    ");
    lcdChar(((getDays() / 10) % 10) + 48); 
    lcdChar((getDays() % 10) + 48); 
    lcdChar('/'); 
    lcdChar(((getMonths() / 10) % 10) + 48); 
    lcdChar((getMonths() % 10) + 48); lcdChar('/'); 
    lcdChar(((getYears() / 10) % 10) + 48); 
    lcdChar((getYears() % 10) + 48);
    
    //Horario                
    lcdCommand(L_L2);
    lcdString("    ");
    lcdChar(((getHours() / 10) % 10) + 48);
    lcdChar((getHours() % 10) + 48);
    lcdChar(':');
    lcdChar(((getMinutes() / 10) % 10) + 48);
    lcdChar((getMinutes() % 10) + 48);
    lcdChar(':');
    lcdChar(((getSeconds() / 10) % 10) + 48);
    lcdChar((getSeconds() % 10) + 48);    
}

void buzzer(void){ //Aciona o buzzer quando finalizar o timer
    pwmSet(100);
    atraso(500);
    pwmSet(0);
    atraso(500);
    pwmSet(100);
    atraso(500);
    pwmSet(0);
    atraso(500);
    pwmSet(100);
    atraso(500);
    pwmSet(0);
}

void atualizaLCD(void){ //FUNCAO PARA ATUALIZAR O LCD
    lcdCommand(L_L1);
    if(modo == 5){lcdString("Pre-Aquec "); lcdPosition(0, 12); lcdChar(1);}
    if(modo == 6){lcdString("Grill "); lcdPosition(0, 12); lcdChar(0);}
    if(modo == 8){lcdString("Manual "); lcdPosition(0, 12); lcdChar(resist);}
    lcdCommand(L_L2);
    lcdString("TEMP:"); //Impressao da temperatura
    lcdPosition(1, 6);
    lcdChar((((250*pot)/1023)+50)/100 %10 + 48); //conversao temp min = 50ºC
    lcdChar((((250*pot)/1023)+50)/10 %10 + 48); //temp max = 300ºC
    lcdChar((((250*pot)/1023)+50) %10 + 48);
    lcdPosition(1, 9);
    lcdChar(0xDF); //simbolo de graus
    lcdString("C");
    if(fim!=0){
        fim = 0;
        state = 0;        
        lcdCommand(L_CLR);
        lcdCommand(L_L1);
        lcdString("      FIM!");
        buzzer();
    }
}

void leTeclado(void){ //Realiza leitura do keypad, chama a funçao correspondente
    static unsigned int tecla = 16;
    
    kpDebounce();
    if(kpRead() != tecla){
        tecla = kpRead();        
        if(bitTst(tecla, 9)){state = 1; lcdCommand(L_CLR);}
        if(state != 0){ //Somente apos ligar
            if(bitTst(tecla, 5)){modo = 5; min = 10; seg = 0; lcdCommand(L_CLR);}//Modo Pre-Aq
            if(bitTst(tecla, 6)){modo = 6; min = 15; seg = 0; lcdCommand(L_CLR);}//Modo Grill
            if(bitTst(tecla, 8)){modo = 8; lcdCommand(L_CLR);}//Modo Manual
            if(bitTst(tecla, 7)){resist++; if(resist>2){resist = 0;}}//Altera Resistencia
            //Controles do timer
            if(start == 0){//Somente timer parado
                if(bitTst(tecla, 0)){min++; if(min>99){min=99;}}//Min+ - Tecla 0
                if(bitTst(tecla, 2)){min--; if(min<0){min=0;}}//Min- - Tecla 2
                if(bitTst(tecla, 3)){seg++; if(seg>59){seg=0; min++; if(min>99){min=99;}}}//Seg+ - Tecla 3
                if(bitTst(tecla, 1)){seg--; if(seg<0){seg=59; min--; if(min<0){min=0;}}}//Seg- - Tecla 1
            }
            if(bitTst(tecla, 4)){start = !start; fim=0;}
            if(modo!=0){
                atualizaLCD();
            }
        }
        
    }
}

void temperatura(void){ //Realiza leitura do pot e atualiza LCD
    static unsigned int ant;
    pot = adcRead(0);
    
    if(ant!=pot){ //Atualiza o LCD somente se houve mudanca
        lcdPosition(1, 6);
        lcdChar((((250*pot)/1023)+50)/100 %10 + 48); //conversao temp min = 50ºC
        lcdChar((((250*pot)/1023)+50)/10 %10 + 48); //temp max = 300ºC
        lcdChar((((250*pot)/1023)+50) %10 + 48);
    }
    
    ant = pot;
}

void escolherModo(void){ //ESCOLHE O MODO DEPOIS DE LIGAR
    lcdCommand(L_L1);
    lcdString("Escolha o modo: ");
    lcdCommand(L_L2);
    lcdString("8-M 5-PAQ 6-Gr");
}

void atualizaSSD(void){ //Atualiza os valores do SSD
    ssdDigit(((min/10) %10), 0);
    ssdDigit(((min/1) %10), 1);
    ssdDigit(((seg/10) %10), 2);
    ssdDigit(((seg/1) %10), 3);
}

void timer(void){ //Realiza a contagem de tempo
    if(start!=0){
        seg--;
        if((seg<0)&&(min>0)){seg=59; min--;}
        if((seg<0)&&(min==0)){seg=0; start=!start; fim=1; atualizaLCD();}
    }
}

void main(void){
    char slot = 0;
    int i;
    unsigned int tempo = 0;
    
    adcInit(); //Inicializa o ADC    
    ssdInit(); //Inicializa o 7-seg display
    lcdInit();//Inicializa o LCD
    kpInit(); //Inicializa o keypad
    timerInit(); //Inicializa o Timer
    pwmInit(); //Inicializa o pwm (buzzer)    
    
    iniSimbolos(); //Coloca os simbolos na memoria
    
    //FORNO AINDA DESLIGADO - MOSTRA HORA E DATA
    while(state==0){
        desligado(); //Mostra data e hora no LCD
        leTeclado(); //Le tecla ligar (9)
    }
    lcdCommand(L_CLR);//Limpa o LDC
    
    //ASSIM QUE LIGAR ESCOLHE UM MODO
    while(modo == 0){
        escolherModo();
        leTeclado();        
    }
    lcdCommand(L_CLR);//Limpa o LCD
    
    for(;;){
    //****************************inicio do top-slot****************************
        timerReset(9000);        
    //***********************inicio da maquina de estado************************
        switch(slot){
            case 0: 
                leTeclado(); //Ler o teclado                
                slot = 1;
                break;
            case 1: 
                temperatura(); //Ler temperatura pelo potenciometro
                slot = 2;                
                break;
            case 2: 
                if(tempo>100){tempo=0; timer();}
                atualizaSSD(); //Atualiza valores no ssd                
                slot = 0;
                break;
            default:
                slot = 0;
                break;
        }
    //**************************inicio do bottom-slot***************************
        if(start!=0){tempo++;}
        ssdUpdate();
        timerWait();        
    }//LOOP
}