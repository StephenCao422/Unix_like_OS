#ifndef IDT_H
#define IDT_H

#define EXCEPTION_SIZE 0x14
#define SYSTEMCALL 0x80
#define KEYBOARD 0x21
#define RTC 0x28
#define PIT 0x20

/* 
 * EXPX/systemcall_blank
 *   DESCRIPTION: Handlers for exceptions and unimplemented system call
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: Prints corresponding exception (or unimplemented system call) and freezes the kernel
 */
void EXP0();
void EXP1();
void EXP2();
void EXP3();
void EXP4();
void EXP5();
void EXP6();
void EXP7();
void EXP8();
void EXP9();
void EXPA();
void EXPB();
void EXPC();
void EXPD();
void EXPE();
void EXPF();
void EXP10();
void EXP11();
void EXP12();
void EXP13();
void systemcall_blank();

/* 
 * idt_init
 *   DESCRIPTION: Initializes interrupt descripter table
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: Fills the entries and loads them into IDT
 */
extern void idt_init();

#endif
