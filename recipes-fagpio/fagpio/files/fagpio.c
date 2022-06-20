#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define MAP_SIZE		0x400							//MMU page size
#define GPIO_REG_BASE		0x01C20800						//GPIO physical base address (small page 4kb)
#define GPIO_BASE_OFFSET	(GPIO_REG_BASE & 0X00000FFF)	//GPIO base address offset calculation
#define GPIO_PAGE_OFFSET	(GPIO_REG_BASE & 0XFFFFF000)	//Get page offset

#define PIO_DAT_OFF		0x10
#define PIO_ADDR_PORT		0x800
#define INPUT				0
#define OUTPUT				1
#define DISABLE				2

#define HIGH				1
#define LOW					0

#define rPE_CFG0			0X90	//PE_CFG0 register address offset
#define rPE_DAT				0XA0	//PE_DAT register address offset
#define rPE_PULL0			0XAC	//PE_PULL0 register address offset
#define rPE_PULL1			0XB0	//PE_PULL1 register address offset

#define BLOCK_SIZE			0x4000

struct cpu_peripheral {
	unsigned long addr_p;
	int mem_fd;
	void *map;
	volatile unsigned int *addr;
};

int fagpio_setup(void);
void fagpio_free(void);

void pinMode(uint8_t Pin, uint8_t Mode);
void digitalWrite(uint8_t pin, uint8_t value);
uint8_t digitalRead(uint8_t pin);




struct cpu_peripheral gpio = {GPIO_PAGE_OFFSET};

// Exposes the physical address defined in the passed structure using mmap on /dev/mem
int map_peripheral(struct cpu_peripheral *p) {
	if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("Failed to open /dev/mem, try checking permissions.\n");
		return -1;
	}

	p->map = mmap(
				NULL,
				BLOCK_SIZE,
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				p->mem_fd,						// File descriptor to physical memory virtual file '/dev/mem'
				p->addr_p						// Address in physical map that we want this memory block to expose
				);

	if (p->map == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	p->addr = (volatile unsigned int *)p->map;

	return 0;
}

void unmap_peripheral(struct cpu_peripheral *p) {
	munmap(p->map, BLOCK_SIZE);
	close(p->mem_fd);
}

/*
PE_CFG0: PE Configure Register 0: Offset: 0x90

PIN         BIT            OUTPUT            INPUT

PE0        2:0              001               000
PE1        6:4              001               000
PE3        10:8             001               000
PE4        14:12            001               000
PE5        22:20            001               000
PE6        26:24            001               000
PE7        30:28            001               000

BIT
32                                            0
  PE7   PE6   PE5   PE4   PE3   PE2   PE1   PE0
0 000 0 000 0 000 0 000 0 000 0 000 0 000 0 000
*/

int fagpio_setup(void) {
	if(map_peripheral(&gpio) == -1) {
		printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
		return -1;
	}
	return 0;
}

void fagpio_free(void) {
	unmap_peripheral(&gpio);
}

void pinMode(uint8_t Pin, uint8_t Mode) {
	if (0 == Mode) {
		printf("Set output\n");
		volatile unsigned int PE_CFG0;
		PE_CFG0 = *(volatile unsigned int *)((unsigned char*)gpio.addr + PIO_ADDR_PORT + rPE_CFG0);
		volatile unsigned int MASK_CONFIG = (~(15 << (4*Pin)));
		volatile unsigned int MASK_X      = (1 << (4*Pin));

		*(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET+rPE_CFG0)=((PE_CFG0 & MASK_CONFIG)|MASK_X);
		printf("([OUTPUT] PE_CFG0 = %08X\n", *(volatile unsigned int *)((unsigned char*)gpio.addr + PIO_ADDR_PORT + rPE_CFG0));
	} else if (1 == Mode) {
		printf("Set input\n");
		volatile unsigned int PE_CFG0, PE_PULL_REG;

		PE_CFG0 = *(volatile unsigned int *)((unsigned char*)gpio.addr + PIO_ADDR_PORT + rPE_CFG0);

		PE_PULL_REG = *(volatile unsigned int *)((unsigned char*)gpio.addr + PIO_ADDR_PORT + rPE_PULL0);

		volatile unsigned int MASK_CONFIG = (~(15 << (4*Pin)));
		volatile unsigned int MASK_X      = (0 << (4*Pin));

		/*
		clear 2bit to 0

		Example:
		00: Pull-up/down disable
		01: Pull-up
		10: Pull-down

					   0xFFFFFFF = 0b 1111 1111 1111 1111 1111 1111 1111 1111 (32 Bit)
					   F = 0b1111 (binary)
					   0x1  1 | 1  1
				  Bit    3  2 | 1  0
							  | Bit 0 and 1, set pull up/down for PE0

						 Bit 2 and 3, set pull up/down for PE1
		*/

		/* clear two bit to zero */
		/* Example: PE2 => 3 << (2*2) => 3 << 4 => 0b00110000 => ~(0b00110000) => 0b11001111 */
		/* volatile unsigned int MASK_PULL = 0b11001111 => 0b1111 1111 1111 1111 1111 1111 1100 1111 */

		volatile unsigned int MASK_PULL    = (~(3 << (Pin*2)));
		volatile unsigned int MASK_XPULL   = (1 << (Pin*2));

		*(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET+rPE_CFG0) = ((PE_CFG0 & MASK_CONFIG)|MASK_X);
		*(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET+rPE_PULL0) = ((PE_PULL_REG & MASK_PULL) | MASK_XPULL);
	}
}

void digitalWrite(uint8_t pin, uint8_t value) {
	if(value == 1) {
		volatile unsigned int PE_DAT;
		volatile unsigned int MASK_CONFIG = (~(1 << (pin)));
		volatile unsigned int MASK_X      = (1 << (pin));
		PE_DAT = *(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET+rPE_DAT);
		*(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET + rPE_DAT)=((PE_DAT & MASK_CONFIG)|MASK_X);
	}
	else if(value == 0) {
		volatile unsigned int PE_DAT;
		volatile unsigned int MASK_CONFIG = (~(1 << (pin)));
		PE_DAT = *(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET+rPE_DAT);
		*(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET + rPE_DAT)=((PE_DAT & MASK_CONFIG));
	}
}

uint8_t digitalRead(uint8_t pin) {
	volatile unsigned int PE_DAT;

	volatile uint8_t value;

	PE_DAT = *(volatile unsigned int *)((unsigned char*)gpio.addr + GPIO_BASE_OFFSET + rPE_DAT);

	value = (uint8_t)(((0x000000FF & PE_DAT) >> pin) & 0x1);

	return value ;
}

//int main(void) {

//	fagpio_setup();

//	pinMode(2, 1);
//	pinMode(3, 1);
//	pinMode(4, 1);
//	pinMode(5, 1);
//	pinMode(6, 1);

//	while(1) {
//		sleep(1);
//		printf("PE2 = %0d\n", digitalRead(2));
//		printf("PE3 = %0d\n", digitalRead(3));
//		printf("PE4 = %0d\n", digitalRead(4));
//		printf("PE5 = %0d\n", digitalRead(5));
//		printf("PE6 = %0d\n", digitalRead(6));
//	}

//	fagpio_free();

//	return 0;
//}













