/*
 * cpu.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-11-24
 *
 * Thanks to sandpile.org for the useful cpu identification tips
 */

#include <cpu.h>
#include <memory.h>
#include <console.h>
#include <string.h>
#include <timer.h>
#include <io.h>
#include <interrupt.h>

/* From x86.asm */
extern int cpu_cpuid_supported(void);
extern void cpu_cpuid_call(unsigned int level, unsigned int *eax, unsigned int *ebx,
				unsigned int *ecx, unsigned int *edx);
extern void _irq0_handler(void);
extern unsigned long long cpu_rdtsc(void);

/* From interrupt.c */
extern void interrupt_set_handler(uint8_t number, void (*function)(void), unsigned access);

				
				
/*******************************************
 * CPU model and capability identification *
 *******************************************/

void cpu_identify_cpuid(void)
{
unsigned int eax, ebx, ecx, edx;
unsigned int max_level, max_ext_level;		/* Maximum CPUID level supported */
unsigned char buffer[64];

	/*
	 * Get standard level information
	 */

	cpu_cpuid_call(0, &max_level, &ebx, &ecx, &edx);

	/* Determine vendor */
	if ((ebx == 0x756e6547) && (edx == 0x49656e69) && (ecx == 0x6c65746e))
		_cpu.vendor = vendorIntel;
	else if ((ebx == 0x68747541) && (edx == 0x69746e65) && (ecx == 0x444d4163))
		_cpu.vendor = vendorAMD;
	else if ((ebx == 0x20434d55) && (edx == 0x20434d55) && (ecx == 0x20434d55))
		_cpu.vendor = vendorUMC;
	else if ((ebx == 0x69727943) && (edx == 0x736e4978) && (ecx == 0x64616574))
		_cpu.vendor = vendorCyrix;
	else if ((ebx == 0x4778654e) && (edx == 0x72446e65) && (ecx == 0x6e657669))
		_cpu.vendor = vendorNexGen;
	else if ((ebx == 0x746e6543) && (edx == 0x48727561) && (ecx == 0x736c7561))
		_cpu.vendor = vendorCentaur;
	else if ((ebx == 0x65736952) && (edx == 0x65736952) && (ecx == 0x65736952))
		_cpu.vendor = vendorRise;
	else if ((ebx == 0x20536953) && (edx == 0x20536953) && (ecx == 0x20536953))
		_cpu.vendor = vendorSiS;
	else if ((ebx == 0x756e6547) && (edx == 0x54656e69) && (ecx == 0x3638784d))
		_cpu.vendor = vendorTransmeta;
	else if ((ebx == 0x646f6547) && (edx == 0x79622065) && (ecx == 0x43534e20))
		_cpu.vendor = vendorNSC;
	else
		_cpu.vendor = vendorUnknown;

	if (max_level < 1)
		return;

	/* Get capabilities and model info */
	cpu_cpuid_call(1, &eax, &ebx, &ecx, &edx);

	_cpu.family = (eax >> 8) & 0xF;
	_cpu.model = (eax >> 4) & 0xF;
	_cpu.stepping = eax & 0xF;

	//console_write_formatted("CPU family: %u, model: %u, stepping: %u\n", _cpu.family, _cpu.model, _cpu.stepping);

	if (edx & (1 << 4))	/* Timestamp counter */
		_cpu.capabilities |= CPU_CAPABILITY_TIMESTAMPCOUNTER;
	if (edx & (1 << 13) ||	/* Global pages. Early AMDs (SSA5) used bit 10 (APIC) to report it */
		( (_cpu.vendor == vendorAMD) && (_cpu.family == 5) && (_cpu.model == 0) && (edx & (1 << 10)) ))
		_cpu.capabilities |= CPU_CAPABILITY_GLOBALPAGES;

	/*
	 * Get extended level information
	 */

	cpu_cpuid_call(0x80000000, &max_level, &ebx, &ecx, &edx);

	/* XXX - Processors that do not support extended levels return
	   indefined values. If they don't return a maximum level above
	   0x80000000 we can suppose it doesn't support them. */
	if ((max_level & 0xFFFF0000) != 0x80000000)
		return;	
}





/*********************
 * Delay calibration *
 *********************/
 
static volatile unsigned int ticks = 0;
static unsigned long delay_count = 1;
 
extern void _calibrate_isr(void);
 
static void calibrate_isr(void)
{				/* The timer ISR                  */
	ticks++;		/* Increment the ticks counter    */
}

static void __delay(unsigned long loops)
{
	while(loops--)
		do {
		}while(0);
}

void delay(unsigned int ms)
{
	__delay((unsigned long)(ms * delay_count));
}

static void cpu_calibrate_delay(void)
{
unsigned int prevtick;	/* Temporary variable             */
unsigned int i;		/* Counter variable               */
unsigned int calib_bit;	/* Bit to calibrate (see below)   */
uint16_t pit_value;

	/* Initialise timer interrupt with 10 ms interval        */
	
	pit_value = 11932;			/* 100 Hz */

	port_write_byte(TIMER_8253_PORT_CONTROL, (3 << 1) | (3 << 4));	/* Square wave generator, load first LSB then MSB, counter 0 */
	port_write_byte(TIMER_8253_PORT_COUNTER0, pit_value & 0xFF);
	port_write_byte(TIMER_8253_PORT_COUNTER0, (pit_value >> 8) & 0xFF);
	
	interrupt_set_handler(0x20, _calibrate_isr, CPU_IDT_ACCESS_PRESENT | CPU_IDT_ACCESS_DPL_0);
	
	interrupt_enable();

	/* Stage 1:  Coarse calibration                                   */

	do {
		delay_count <<= 1;		/* Next delay count to try        */

		prevtick = ticks;		/* Wait for the start of the next */
		while (prevtick == ticks);	/* timer tick                     */

		prevtick = ticks;		/* Start measurement now          */
		__delay(delay_count);		/* Do the delay                   */
	} while (prevtick == ticks);		/* Until delay is just too big    */

	delay_count >>= 1;			/* Get bottom value for delay     */

	/* Stage 2:  Fine calibration                                     */

	calib_bit = delay_count;		/* Which bit are we going to test */

	for (i = 0; i < 8; i++) {
		calib_bit >>= 1;		/* Next bit to calibrate          */
		if (!calib_bit)
			break;			/* If we have done all bits, stop */

		delay_count |= calib_bit;	/* Set the bit in delay_count     */

		prevtick = ticks;		/* Wait for the start of the next */
		while (prevtick == ticks);	/* timer tick                     */

		prevtick = ticks;		/* Start measurement now          */
		__delay(delay_count);		/* Do the delay                   */

		if (prevtick != ticks)		/* If a tick has passed, turn the */
			delay_count &= ~calib_bit;	/* calibrated bit back off */
	}

	/* We're finished:  Do the finishing touches                      */

	interrupt_disable();
	interrupt_set_handler(0x20, _irq0_handler, CPU_IDT_ACCESS_PRESENT | CPU_IDT_ACCESS_DPL_0);
	
	delay_count /= 10000;			/* Calculate delay_count for 1ms   */
}

/*
 * The hardware part of the timer ISR:
 * This code is invoked when the PIT finishes its period.
 */

asm(".globl _calibrate_isr   \n" 
	"_calibrate_isr:        \n" 
	"   pusha               \n"	/* Save all registers             */
    	"                       \n" 
	"   call calibrate_isr  \n"	/* Call the actual ISR code       */
    	"                       \n" 
	"   movb $0x20,%al      \n"	/* Send EOI to the 8259           */
    	"   outb %al,$0x20      \n" 
    	"                       \n" 
	"   popa                \n"	/* Restore registers              */
    	"   iret                \n"	/* Return from interrupt          */
);
	
	
	
	
/**************************
 * General initialization *
 **************************/

err_t cpu_init(void)
{
	memory_clear(&_cpu, sizeof(_cpu));

	/*
	 * Get CPU capabilities and info.
	 * First of all, check if the processor supports the CPUID instruction
	 *
	 * TODO: Cyrix and NexGen processor need to have the CPUID instruction
	 * enabled before use.
	 */
	
	if (cpu_cpuid_supported())
		cpu_identify_cpuid();
	else {
		//console_write("\t-CPUID not supported by this processor-\n");
		_cpu.vendor = vendorUnknown;
		_cpu.family = _cpu.model = _cpu.stepping = -1;
		
		return 1;
	}
	
	cpu_calibrate_delay();

	return 0;
}
