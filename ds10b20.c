/*
 * onewire.c
 *
 *  Created on: 21/8/2019
 *      Author: usuario
 */
#include "ds18b20.h"

//**********************************************************************************************************************************************************
void ds18b20_init_port(void) {
    DS18B20_PORT_DIR |= DS18B20_PORT_PIN;
    DS18B20_PORT_OUT |= DS18B20_PORT_PIN;
    // Typical pull-up/down resistor from MSP430 is 35kOhm, too large for the ideal value ~5kOhm
    DS18B20_PORT_REN |= DS18B20_PORT_PIN;
}

//**********************************************************************************************************************************************************
uint8_t ds18b20_reset(void)
{
    DS18B20_LO
    delay_us(500);                  // 480us minimum
    DS18B20_RLS
    delay_us(60);                   // slave waits 15-60us  // try 80 or 40
    if (DS18B20_IN) return 1;       // line should be pulled down by slave
    delay_us(240);                  // slave TX presence pulse 60-240us
    if (!DS18B20_IN) return 2;      // line should be "released" by slave
    return 0;
}

//**********************************************************************************************************************************************************
void ds18b20_write_bit(uint8_t bit)
{
    DELAY_US(1);                    // recovery, min 1us
    DS18B20_HI
    if (bit) {
        DS18B20_LO
        DELAY_US(5);                // max 15us
        DS18B20_RLS                 // input
        delay_us(50);
    }
    else {
        DS18B20_LO
        delay_us(60);               // min 60us
        DS18B20_RLS                 // input
        DELAY_US(1);
    }
}

//**********************************************************************************************************************************************************
uint8_t ds18b20_read_bit()
{
    uint8_t bit=0;
    DELAY_US(1);
    DS18B20_LO
    DELAY_US(1);                    // hold min 1us
    DS18B20_RLS
    DELAY_US(5);                    // 15us window
    if (DS18B20_IN) bit = 1;
    delay_us(45);                   // rest of the read slot
    return bit;
}

//**********************************************************************************************************************************************************
void ds18b20_write_byte(uint8_t byte)
{
    uint8_t i;
    for(i = 8; i > 0; i--)
    {
        ds18b20_write_bit(byte & 1);
        byte >>= 1;
    }
}

//**********************************************************************************************************************************************************
uint8_t ds18b20_read_byte()
{
    uint8_t i;
    uint8_t byte = 0;
    for(i = 8; i > 0; i--)
    {
        byte >>= 1;
        if (ds18b20_read_bit()) byte |= 0x80;
    }
    return byte;
}

//**********************************************************************************************************************************************************
uint16_t ds18b20_read_temp_registers ( void )
{
    uint8_t i;
    uint16_t byte = 0;
    for(i = 16; i > 0; i--){
        byte >>= 1;
        if (ds18b20_read_bit()) {
            byte |= 0x8000;
        }
    }
    return byte;
}

//**********************************************************************************************************************************************************
uint64_t readRom (void){
    uint8_t i;
    uint64_t byte = 0;

    ds18b20_reset();
    ds18b20_write_byte(DS1820_READ_ROM);

    for(i = 64; i > 0; i--){
        byte >>= 1;
        if (ds18b20_read_bit()) {
            byte |= 0x8000000000000000;
        }
    }
    return byte;
}

//**********************************************************************************************************************************************************
float ds18b20_get_temp(void)
{
    volatile uint16_t temp = 0;
    ds18b20_reset();
    ds18b20_write_byte(DS18B20_SKIP_ROM);           // skip ROM command
    ds18b20_write_byte(DS18B20_CONVERT_T);          // convert T command
    DS18B20_HI
    delay_ms(800);                                  // at least 750 ms for the default 12-bit resolution
    ds18b20_reset();
    ds18b20_write_byte(DS18B20_SKIP_ROM);           // skip ROM command
    ds18b20_write_byte(DS18B20_READ_SCRATCHPAD);    // read scratchpad command
    temp = ds18b20_read_temp_registers();

    return(temp / 16.0);
}

//**********************************************************************************************************************************************************
void show_temp(float temp)
{
    uint16_t aux;

    aux=temp/10;
    showChar(aux+48,pos1);
    aux=(int16_t)temp%10;
    showChar(aux+48,pos2);
    LCDMEM[pos2+1] |= 0x01;

    volatile float mantisa = temp - (int16_t)temp;
    volatile uint16_t dosDecimales = mantisa * 100;
    aux=((int)dosDecimales)/10;
    showChar(aux+48,pos3);
    aux=((int)dosDecimales)%10;
    showChar(aux+48,pos4);
}

//**********************************************************************************************************************************************************
void delay_us(uint16_t us)
{
    TA0CTL = TACLR;
    TA0CCR0 = us;
    TA0CCTL0 |= CCIE;
    TA0EX0 = TAIDEX_7;
    TA0CTL = TASSEL_2 + ID_0 + MC_1;
    __bis_SR_register(LPM3_bits + GIE);
}

//**********************************************************************************************************************************************************
void delay_ms(uint16_t ms)
{
    TA0CTL = TACLR;
    TA0CCR0 = ms;
    TA0CCTL0 |= CCIE;
    TA0EX0 = TAIDEX_7;
    TA0CTL = TASSEL_1 + ID_2 + MC_1;
    __bis_SR_register(LPM3_bits + GIE);
}

//**********************************************************************************************************************************************************
void set_clock(){

    __bis_SR_register(SCG0);                    // disable FLL
    CSCTL3 |= SELREF__REFOCLK;                  // Set REFO as FLL reference source
    CSCTL0 = 0;                                 // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                     // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_3;                        // Set DCO = 8MHz
    CSCTL2 = FLLD_0 + 243;                      // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                    // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));  // Poll until FLL is locked
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;  // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz                                             // default DCODIV as MCLK and SMCLK source
}

//********************************************************************************************************************************************************************
// Timer A0 interrupt service routine --> Timer0_A3 CC0
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{
    TA0CTL = 0;
    TA0CCTL0 = 0;
    TA0CCR0 = 0;
    TA0EX0 = TAIDEX_0;
    __bic_SR_register_on_exit(LPM3_bits + GIE);
}
