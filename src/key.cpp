#include <unistd.h>
#include <librpitx/librpitx.h>
#include "stdio.h"
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

bool running=true;

#define PROGRAM_VERSION "0.1"

//#define BUTTON_PIN 26

#define BCM_BASE_ADDRESS 0x3F000000  // base address
#define GPIO_BASE_OFFSET 0x200000    // controller offset
#define GPIO_PIN_OFFSET  26          // pin 26

#define REG_BLOCK_SIZE   4096        // register block size


void print_usage(void)
{

fprintf(stderr,\
"\ntune -%s\n\
Usage:\ntune  [-f Frequency] [-h] \n\
-f float      frequency carrier Hz(50 kHz to 1500 MHz),\n\
-e exit immediately without killing the carrier,\n\
-p set clock ppm instead of ntp adjust\n\
-h            help (this help).\n\
\n",\
PROGRAM_VERSION);

} /* end function print_usage */

static void
terminate(int num)
{
    running=false;
	fprintf(stderr,"Caught signal - Terminating\n");
   
}

int main(int argc, char* argv[])
{
	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Failed to open /dev/mem. Make sure to use sudo.");
        return 1;
    }
	int a;
	int anyargs = 0;
	float SetFrequency=434e6;
	dbg_setlevel(1);
	bool NotKill=false;
	float ppm=1000.0;

	// map physical memory to virtual memory
	void* gpio_base = mmap(
        NULL,
        REG_BLOCK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        BCM_BASE_ADDRESS + GPIO_BASE_OFFSET
    );
    if (gpio_base == MAP_FAILED) {
        perror("Failed to mmap GPIO.");
        close(mem_fd);
        return 1;
    }

	// setting pin 26 as input
	volatile unsigned int* gpio = (volatile unsigned int*)gpio_base;
    gpio[GPIO_PIN_OFFSET / 10] &= ~(3 << ((GPIO_PIN_OFFSET % 10) * 3));

	while(1)
	{
		a = getopt(argc, argv, "f:ehp:");
	
		if(a == -1) 
		{
			if(anyargs) break;
			else a='h'; //print usage and exit
		}
		anyargs = 1;	

		switch(a)
		{
		case 'f': // Frequency
			SetFrequency = atof(optarg);
			break;
		case 'e': //End immediately
			NotKill=true;
			break;
		case 'p': //ppm
			ppm=atof(optarg);
			break;	
		case 'h': // help
			print_usage();
			exit(1);
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 			{
 				fprintf(stderr, "tune: unknown option `-%c'.\n", optopt);
 			}
			else
			{
				fprintf(stderr, "tune: unknown option character `\\x%x'.\n", optopt);
			}
			print_usage();

			exit(1);
			break;			
		default:
			print_usage();
			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */

	
	
	 for (int i = 0; i < 64; i++) {
        struct sigaction sa;

        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

		generalgpio gengpio;
		gengpio.setpulloff(4);
		padgpio pad;
		pad.setlevel(7);
		clkgpio *clk=new clkgpio;
		clk->SetAdvancedPllMode(true);
		if(ppm!=1000)	//ppm is set else use ntp
			clk->Setppm(ppm);
		clk->SetCenterFrequency(SetFrequency,10);
		clk->SetFrequency(000);
		
		//clk->enableclk(6);//CLK2 : experimental
		//clk->enableclk(20);//CLK1 duplicate on GPIO20 for more power ?
		if(!NotKill)
		{
			while(running)
			{
				if ((gpio[GPIO_PIN_OFFSET / 32] & (1 << (GPIO_PIN_OFFSET % 32))) == 0) {

					clk->enableclk(4);

					printf("\nButton Pressed");

            		// Wait for button release
           			while ((gpio[GPIO_PIN_OFFSET / 32] & (1 << (GPIO_PIN_OFFSET % 32))) == 0) {
                		// Wait for button release
           			}

					clk->disableclk(4);

					usleep(1000);
      			}

				/*
				if (gpio[GPIO_PIN_OFFSET / 32] & (1 << (GPIO_PIN_OFFSET % 32))) {
            		
					clk->enableclk(4);

					printf("\nButton Pressed");
					
					// waiting for button to be released
            		while (gpio[GPIO_PIN_OFFSET / 32] & (1 << (GPIO_PIN_OFFSET % 32))) {
						
            		}

					clk->disableclk(4);
					usleep(10000);
       			}
				*/
			}

			munmap(gpio_base, REG_BLOCK_SIZE);
    		close(mem_fd);

			clk->disableclk(4);
			clk->disableclk(20);

			delete(clk);

			return(0);
		}
		else
		{
			//Ugly method : not destroying clk object will not call destructor thus leaving clk running 
		}
	
	
}	

