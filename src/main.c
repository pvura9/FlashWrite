#include "diary.h"
#include <stdlib.h>
#include "stm32f0xx.h"
#include <stdio.h>
#include <string.h>
#include "eepromDriver.h"
#include "fifo.h"
#include "tty.h"
#include "serial.h"
#include "rtc.h"

//just set to 5423 temporarily for testing
#define PASSWORD "5423"
#define MAX_PW_ATTEMPTS 3
#define FIFOSIZE 16

char serfifo[FIFOSIZE];
int seroffset = 0;
volatile uint32_t msTicks = 0;
void internal_clock();

//password verification logic
int verifyPassword()
{
    char input[32];
    int attempts = 0;
    while(attempts < MAX_PW_ATTEMPTS)
    {
        printf("\r\nPlease enter the password: ");
        gets(input);
        if(strcmp(input, PASSWORD) == 0)
        {
            return 1;
        }
        attempts++;
        printf("\r\nIncorrect password (%d / %d attempts)", attempts, MAX_PW_ATTEMPTS);
    }
    printf("\r\nMax attempts reached. System locked.");
    return 0;
}


void enable_tty_interrupt(void) 
{
    /*
    each time a char is received:
    1. raise an interrupt every time the receive data register becomes not empty
        - set the proper bit in the nvic iser as well
    2. trigger a dma operation every time the receive data register becoems not empty
        - do so by enabling dma mode for reception
    */
    USART5->CR1 |= USART_CR1_RXNEIE;
    NVIC->ISER[0] |= (1 << USART3_8_IRQn);
    USART5->CR3 |= USART_CR3_DMAR;

    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->CSELR |= DMA2_CSELR_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;

    DMA2_Channel2->CMAR = (uint32_t) serfifo;
    DMA2_Channel2->CPAR = (uint32_t) &(USART5->RDR);
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;
    DMA2_Channel2->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE);
    DMA2_Channel2->CCR |= DMA_CCR_MINC; 
    DMA2_Channel2->CCR |= DMA_CCR_CIRC;
    DMA2_Channel2->CCR |= DMA_CCR_PL;
    DMA2_Channel2->CCR |= DMA_CCR_TCIE;
    DMA2_Channel2->CCR |= DMA_CCR_EN;
}

//works like line_buffer_getchar(), but does not check or clear ORE nor wait on new characters in USART
char interrupt_getchar() 
{
    while(fifo_newline(&input_fifo) == 0) 
    {
        asm volatile ("wfi");
    }
    // Return a character from the line buffer.
    char ch = fifo_remove(&input_fifo);
    return ch;
}

void USART3_8_IRQHandler(void) 
{
    while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) 
    {
        if (!fifo_full(&input_fifo))
        {
            insert_echo_char(serfifo[seroffset]);
        }
        seroffset = (seroffset + 1) % sizeof serfifo;
    }
}

void init_usart5() 
{
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN; //clk for gpioc
    RCC->AHBENR |= RCC_AHBENR_GPIODEN; //clk for gpiod
    RCC->APB1ENR |= RCC_APB1ENR_USART5EN; //clk for usart5 

    GPIOC->MODER &= ~(3 << (24)); //clr for pc12
    GPIOC->MODER |= (2 << (24)); //set to af 
    GPIOC->AFR[1] &= ~(0b1111 << 16); //clr af bits
    GPIOC->AFR[1] |= (0b0010 << 16); //set af2

    GPIOD->MODER &= ~(3 << 4); //clr for pd2
    GPIOD->MODER |= (2 << 4); //set to af
    GPIOD->AFR[0] &= ~(0b1111 << 8); //clr af
    GPIOD->AFR[0] |= (0b0010 << 8); //set af2

    USART5->CR1 &= ~USART_CR1_UE; //disable
    USART5->CR1 &= ~USART_CR1_M; //set word len to 8
    USART5->CR1 &= ~USART_CR1_PCE; //disable parity
    USART5->CR2 &= ~USART_CR2_STOP; //set stop bit
    USART5->CR1 &= ~USART_CR1_OVER8; //set to 16x oversampling

    USART5->BRR = (48000000 / 115200); //set baud rate
    USART5->CR1 |= USART_CR1_TE; //enable transmitter
    USART5->CR1 |= USART_CR1_RE; //enable receiver
    USART5->CR1 |= USART_CR1_UE; //enable usart5

    while (!(USART5->ISR & USART_ISR_TEACK)); //CHECK REACK as well
}



int __io_putchar(int c) 
{
    if (c == '\n') 
    {  
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';
    }
    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) 
{
    char getChar = interrupt_getchar();
    return getChar;
}


int main(void) 
{
    //all clock and peripheral initializations 
    SystemCoreClockUpdate();
    internal_clock();
    init_usart5();
    enable_tty_interrupt();
    rtcInit();

    //turn off the buffering - first 1023 chars are displayed this way
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    //opening print statements
    printf("\n ");
    printf("\r\n=== Digital Diary System ===");

    //keep locked while password not verified
    if(!verifyPassword())
    {
        //lock system
        while(1);
    }
    printf("\r\nAccess granted!\n");

    //initialize the eeprom
    printf("\r\nInitializing EEPROM...\r\n");
    flashUnlock();
    flashErasePage(FLASH_PAGE_62_ADDRESS);
    flashErasePage(FLASH_PAGE_63_ADDRESS);
    flashLock();

    //after EEPROM initialization
    printf("\rInitializing memory system...\n");
    printf("\r\nDiary System Ready");
    
    //input parsing logic
    while(1) 
    {
        printf("\r\n> ");
        char cmd[32];
        gets(cmd);
        
        if(strncmp(cmd, "write", 5) == 0) 
        {
            handleWriteCommand();
        }
        else if(strncmp(cmd, "search ", 7) == 0) 
        {
            handleSearchCommand(cmd + 7);
        }
        else if(strncmp(cmd, "read ", 5) == 0) 
        {
            handleReadCommand(atoi(cmd + 5));
        }
        else if(strncmp(cmd, "delete ", 7) == 0) 
        {
            handleDeleteCommand(atoi(cmd + 7));
        }
        else if(strncmp(cmd, "list", 4) == 0) 
        {
            handleListCommand();
        }
        else if (strncmp(cmd, "logout", 6) == 0) 
        {
            handleLogoutCommand();
        }
        else 
        {
            printf("\r\nAvailable commands:");
            printf("\r\n  write - Create new entry");
            printf("\r\n  search <tag> - Find entries by tag");
            printf("\r\n  read <index> - Read entry by index");
            printf("\r\n  delete <index> - Delete entry by index");
            printf("\r\n  list - Show all entries");
            printf("\r\n  logout - Exit the diary system");
        }
    }
}