// All necessary declarations for the Tux Controller driver must be in this file

#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)


// Local fncs
extern int tuxctl_ioctl_tux_init(struct tty_struct* );
extern int tuxctl_ioctl_tux_buttons(struct tty_struct* , unsigned long );
extern int tuxctl_ioctl_tux_set_led(struct tty_struct* , unsigned long );


#endif

