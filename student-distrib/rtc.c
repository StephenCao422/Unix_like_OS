#include "rtc.h"


/*
* rtc_init
*   DESCRIPTION: Initialize the RTC
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: unmarks the rtc iqr
*/
void rtc_init(){
    cli();
    outb(REG_B, RTC_COMMAND); // Select Register B, and disable NMI (by setting the 0x80 bit)
    char prev = inb(RTC_DATA); // Read the current value of Register B
    outb(REG_B, RTC_COMMAND);
    outb(prev | 0x40, RTC_DATA); // Turn on bit 6 of register B to enable periodic interrupts
    enable_irq(RTC_IRQ);
    sti();
    rtc_set_rate(0);
}

/*
* rtc_handler
*   DESCRIPTION: Handles the rtc interrupt
*   INPUTS: none
*   OUTPUTS: none
*   RETURN VALUE: none
*   SIDE EFFECTS: reads the register C and sets the flag to indicate interrupt occurred
*/
void rtc_handler(){
    cli();
    outb(REG_C, RTC_COMMAND); // Select and flush Register C
    inb(RTC_DATA);
    printf("RTC interrupt\n");
    send_eoi(RTC_IRQ);
    sti();
}

/*
* rtc_set_rate
*   DESCRIPTION: Set the rate of the RTC
*   INPUTS: rate - the rate to set the RTC to
*   OUTPUTS: none
*   RETURN VALUE: 0 if successful, -1 if not
*   SIDE EFFECTS: changes the frequency of the RTC
*/
int32_t rtc_set_rate(int32_t rate){
    if (rate < 0 || rate > 15){
        return -1; // can only be within 0-15
    }
    cli();
    outb(REG_A, RTC_COMMAND);  // Select Register A, but do not disable NMI (0x80 bit is not set)
    char prev = inb(RTC_DATA); // Read the current value of Register A
    char new_value = (prev & 0xF0) | (rate); // Prepare to write the new rate by preserving the top 4 bits and updating the bottom 4 bits
    outb(REG_A, RTC_COMMAND); // Write the new rate value back to Register A
    outb(new_value, RTC_DATA); // Re-select Register A in case the index was changed
    sti();
    return 0;
}

int32_t rtc_open(const uint8_t* filename){
    return 0;
}

int32_t rtc_close(int32_t fd){
    return 0;
}

int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    return 0;
}

int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    return 0;
}
