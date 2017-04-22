// Morse Code Translater
// 3/6/17
#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// helper functions
void UARTSendArray(unsigned char *TxArray, unsigned char ArrayLength);
void Init_UART(void);

static char string[62];
static const char *pattern[] = {"10111", "111010101", "11101011101", "1110101",
                        "1", "101011101", "111011101", "1010101", "1010",
                        "1011101110111", "111010111", "101110101", "1110111",
                        "11101", "11101110111", "10111011101", "1110111010111",
                        "1011101", "10101", "111", "1010111", "101010111",
                        "101110111", "11101010111", "1110101110111",
                        "11101110101", "0000"};

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;   // stop WDT

    P1DIR = 0x01;            // set P1 direction
    P1OUT = 0x00;            // turn LED off

    Init_UART();

    TACCR0  =   31249;          // set count top
    TACTL   =   TASSEL1 |       // clock source selection - use SMCLK
                    MC0 |       // up mode - count to TACCR0, then reset
                    ID_3;       // clock divider - divide ACLK by 8

    // enter LPM0, interrupts enabled
    __bis_SR_register(LPM0_bits + GIE);
}

/* ISR for recieving the input string */
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    static int index = 0, count = 0;
    static char data;

    data = UCA0RXBUF;
    data = tolower(data);

    if (data != '\r' && count != 61)
    {
        if (data > 96 && data < 123)
        {
            string[index++] = data;
            count++;
        }
        else
            count++;
    }
    else
    {
        // store carriage return
        string[index] = data;
        // reset variables
        index = 0;
        count = 0;
        // echo string to terminal
        UARTSendArray(&string, 62);
        // disable the UART interrupt
        IE2 &= ~UCA0RXIE;
        // enable the timer interrupt
        TACCTL0 = CCIE;           // enable capture/compare interrupt
                                  // capture/compare value register
    }
}

/* Timer A Interrupt - Morse Code Blink */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer0_isr(void)
{
    static int string_position = 0;
    static int pattern_select = 0;
    static int pattern_position = 0;
    int i = 0;

    // find which morse code pattern to blink
    pattern_select = string[string_position] - 'a';

    // if the character is space, select the space pattern
    if (string[string_position] == 32)
        pattern_select = 26;

    if (pattern[pattern_select][pattern_position] == '1')
    {
        P1OUT = 0x01;
        pattern_position++;
    }
    else if (pattern[pattern_select][pattern_position] == '0')
    {
        P1OUT = 0x00;
        pattern_position++;
    }
    else if (pattern[pattern_select][pattern_position] == 0x00)
    {
        string_position++;
        pattern_position = 0;
    }
    else if (string[string_position] == 0x0D || string_position == 63)
    {
        for (i = 0; i < string_position; i++)
            string[i] = "";

        string_position = 0;
        pattern_position = 0;

        P1OUT = 0x00;

        // enable the UART interrupt
        IE2 |= UCA0RXIE;
        // disable the timer interrupt
        TACCTL0 &= ~CCIE;
    }
}

void Init_UART(void)
{
    /* Use Calibration values for 1MHz Clock DCO */
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    /* Configure Pin Muxing P1.1 RXD and P1.2 TXD */
    P1SEL = BIT1 | BIT2 ;
    P1SEL2 = BIT1 | BIT2;

    /* Place UCA0 in Reset to be configured */
    UCA0CTL1 = UCSWRST;

    /* Configure */
    UCA0CTL1 |= UCSSEL_2;       // SMCLK
    UCA0BR0 = 104;              // 1MHz 9600
    UCA0BR1 = 0;                // 1MHz 9600
    UCA0MCTL = UCBRS0;          // modulation UCBRSx = 1

    /* Take UCA0 out of reset */
    UCA0CTL1 &= ~UCSWRST;

    /* Enable USCI_A0 RX interrupt */
    IE2 |= UCA0RXIE;
}

void UARTSendArray(unsigned char *TxArray, unsigned char ArrayLength){

    while(ArrayLength--)
    {                                   // loop until StringLength == 0 and post decrement
        while(!(IFG2 & UCA0TXIFG));     // wait for TX buffer to be ready for new data
        UCA0TXBUF = *TxArray;           // write the character at the location specified py the pointer
        TxArray++;                      // increment the TxString pointer to point to the next character
    }
}
