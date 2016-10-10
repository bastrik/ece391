/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__);

/************************ Local Variables *************************/

static spinlock_t button_lock = SPIN_LOCK_UNLOCKED;
int spam_defender;

struct State {
	/* {UP, DOWN, LEFT, RIGHT, A, B, C, START} */
	unsigned long buttons;
	unsigned long display;
} state;

unsigned char LED[16] = 
{0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
    unsigned int packet1, packet2;
    unsigned char top;
    unsigned long irqflags;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    /*printk("packet : %x %x %x\n", a, b, c); */

    switch(a)
    {
    	case MTCP_ACK:
    		spam_defender = 1;
    		return;
    	case MTCP_BIOC_EVENT:
    		// reverse the order 
    		packet1 = ~b & 0x0F;
    		packet2 = ~c & 0x0F;
    		top = (((packet2 & 0x09) | ((packet2 & 0x02) << 1))) | ((packet2 & 0x04) >> 1);

    		spin_lock_irqsave(&button_lock, irqflags); 	// critical section begins
    		state.buttons = (top << 4) | packet1;
    		spin_unlock_irqrestore(&(button_lock), irqflags);	// critical section ends

    		return;
    	case MTCP_RESET:
    		tuxctl_ioctl_tux_init(tty);
    		if (spam_defender)
    			tuxctl_ioctl_tux_set_led(tty, state.display);	// Restore LED status
    		return;
    	default: 
    		return;    		
    }
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		return tuxctl_ioctl_tux_init(tty);
	case TUX_BUTTONS:
		return tuxctl_ioctl_tux_buttons(tty, arg);
	case TUX_SET_LED:
		return tuxctl_ioctl_tux_set_led(tty, arg);
	case TUX_LED_ACK:
		return 0;
	case TUX_LED_REQUEST:
		return 0;
	case TUX_READ_LED:
		return 0;
	default:
	    return -EINVAL;
    }
}

/* 
 *	tuxctl_ioctl_tux_init
 * 	DESCRIPTION: Initialize the Tux Controller by
 * 				 sending initialization signal to the controller
 *  INPUTS:      tty: A pointer to tty_struct
 *  OUTPUTS: none
 *  RETURN VALUE: 0 on success
 *  SIDE EFFECTS: 
 */
 int tuxctl_ioctl_tux_init(struct tty_struct* tty)
 {
 	unsigned char commands[2] = {MTCP_BIOC_ON, MTCP_LED_USR};

 	spam_defender = 0;
 	// we want to initialize (reset) our controller
 	state.buttons = 0x0;
 	state.display = 0x0;	

 	tuxctl_ldisc_put(tty, &commands[0], 1);
 	tuxctl_ldisc_put(tty, &commands[1], 1);

 	return 0;
 }

/* 
 * tuxctl_ioctl_tux_buttons
 *   DESCRIPTION: Copy TUX button status to user space
 *   INPUTS:	  tty:	A tty pointer
 * 			 	  arg: 	A long containing pointer to the address of the button status  
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *   SIDE EFFECTS:
 */
 int tuxctl_ioctl_tux_buttons(struct tty_struct* tty, unsigned long arg)
 {
 	unsigned long irqflags;
 	unsigned long kernel = (unsigned int)&state.buttons;
	int ret_val;

	// if arg passed in was NULL, return
	if (!arg)
		return -EINVAL; 

	spin_lock_irqsave(&button_lock, irqflags);	//protect the data
	ret_val = copy_to_user((void *)arg, (void *)kernel, sizeof(long));
	spin_unlock_irqrestore(&button_lock, irqflags); //critical section ends

 	if(ret_val > 0)
		return -EINVAL;
	else
		return 0;
 }

/* 
 * tuxctl_ioctl_tux_set_led
 *   DESCRIPTION: Display data from arg on the Tux LED display
 *   INPUTS:	  tty:	A tty pointer
 * 			 	  arg: 	A 32 bit int:
 						low 16 bit spicify a number in hex to be displayed
						low 4  bit of 3rd byte specifies which LED are on
						low 4  bit of 4th byte specifies where the decimal point is
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success
 *   SIDE EFFECTS:
 */
 int tuxctl_ioctl_tux_set_led(struct tty_struct* tty, unsigned long arg)
 {
 	int i, led_index, offset;
 	unsigned char led_setup[6]; // # of LEDs + 2

 	unsigned char led_on;		// these are information in arg
	unsigned char decimal;
	unsigned char led_digits[4];

	unsigned long bitmask = 0x000F;

	if (!spam_defender)
		return -1;
	spam_defender = 0;

	for (i = 0; i < 4; i++)
	{	
		offset = 4 * i;
		led_digits[i] = (arg & bitmask) >> (offset); 	// shift 4 bits
		bitmask <<= 4;
	}

	led_on = (arg & 0x000F0000) >> 16;	// parse arg so we can use
	decimal = (arg & 0x0F000000) >> 24;

	led_setup[0] = MTCP_LED_USR; 	// set LED to user mode
 	tuxctl_ldisc_put(tty, &led_setup[0], 1);

 	led_setup[0] = MTCP_LED_SET;
 	led_setup[1] = led_on;

 	bitmask = 0x1;
 	led_index = 2;
 	for (i = 0; i < 4; i++)
 	{
 		if (led_on & bitmask)
 		{
 			led_setup[led_index] = LED[led_digits[i]];
 			if (decimal & bitmask)
 				led_setup[i] |= 0x10;
 			led_index++;
 		}
 		bitmask <<= 1;
 	}

	state.display = arg;	// save this as current display
	tuxctl_ldisc_put(tty, led_setup, 6);

	return 0;
 }

