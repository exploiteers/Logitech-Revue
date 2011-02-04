/* drivers/misc/ka-pic.c
 *
 * Copyright (C) 2010 Logitech
 *
 * Richard Titmuss <richard_titmuss@logitech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/poll.h>

#include <asm/uaccess.h>


/* line discipline (mouse is not used) */
#define N_PIC24F		N_MOUSE


#define PIC_MAX_LEN 		42

#define PIC_CMD_ACK		0x06
#define PIC_CMD_NACK 		0x07
#define PIC_CMD_START		0x08
#define PIC_CMD_VERSION		0x09
#define PIC_CMD_IRCAP_START	94
#define PIC_CMD_IRTX_START	92
#define PIC_CMD_IRTX_STOP	95
#define PIC_CMD_IRTX_FINISHED	96
#define PIC_CMD_BUTTON          97
#define PIC_CMD_LISTEN_START    98
#define PIC_CMD_LISTEN_DATA     99
#define PIC_CMD_IR_BEAT		100

#define PIC_CMD_GIGABYTE	63
#define PIC_CMD_FIRMWARE	65
#define PIC_CMD_CHG_STATE	66
#define PIC_CMD_WDT		70
#define PIC_CMD_SYSTEM_FLAG	71
#define PIC_CMD_LED_CON		72
#define PIC_CMD_FAN_CTRL_CON	73
#define PIC_CMD_FAN_CTRL_GET	74

#define PIC_MIDDLEWARE_REGISTRATION 	0x02

	
#define IRCAP_BUF_SIZE		512
/* Max lenth of IR data you can recieve */
#define IRLISTEN_BUF_SIZE	32*5
#define TIMEOUT			10 * HZ /* 10 seconds */


#define PIC24_IOC_MAGIC		('L' | 0x80)

#define PIC24_IOCIRGENSTOP	_IO(PIC24_IOC_MAGIC, 0)
#define PIC24_IOCIRGENFINISHED	_IO(PIC24_IOC_MAGIC, 1)
#define PIC24_IOCIRCAPSTART	_IO(PIC24_IOC_MAGIC, 2)
#define PIC24_IOCIRCAPSTOP	_IO(PIC24_IOC_MAGIC, 3)
#define PIC24_IOCIRBEAT  	_IO(PIC24_IOC_MAGIC, 4)
#define PIC24_IOCIRLISTENSTART _IO(PIC24_IOC_MAGIC, 5)
#define PIC24_IOCIRLISTENSTOP  _IO(PIC24_IOC_MAGIC, 6)

#define PIC24_IOC_MAXNR		7


struct pic24f_device {
	struct miscdevice	ir_device;
	struct miscdevice	ircap_device;
	struct miscdevice	cec_device;
	struct miscdevice	wdog_device;
	struct miscdevice	irlisten_device;
	struct input_dev	*input_dev;
	struct tty_struct 	*tty;
	wait_queue_head_t	wait;
	struct semaphore	lock;

	unsigned int		parser_state;
	unsigned char		rx_buf[PIC_MAX_LEN];
	size_t			rx_ptr, rx_len;
	unsigned char		rx_checksum, checksum;

	bool			ircap;
	wait_queue_head_t	ircap_wait;
	struct semaphore	ircap_lock;
	char                    *ircap_buf;
	char                    *ircap_rp, *ircap_wp;

	struct semaphore	irlisten_lock;
	wait_queue_head_t	irlisten_wait;
	char               *irlisten_buf;
	char               *irlisten_rp, *irlisten_wp;

	unsigned char		acknack;
	char			version[20]; // TODO max length?
	int			cputemp;
	int			fanrpm;
	int			irtx_finished;
};

static struct pic24f_device pic24f_device;

static void pic24f_msg_process(struct pic24f_device *pic24f);




static char int_to_hex(unsigned int n)
{
	if (n <= 9) {
		return n + '0';
	}
	else if (n <= 15) {
		return n + 'A' - 10;
	}
	return '0';
}


static unsigned int hex_to_int(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return 0;
}


/* format message and send it to the pic */
static int pic24f_tty_tx(struct tty_struct *tty, const unsigned char *buf, size_t len)
{
	int i;
	unsigned char checksum = 0;
	size_t out_len = len * 2 + 4;
	char *ptr, out[PIC_MAX_LEN];

	if (len > PIC_MAX_LEN) {
		printk(KERN_ERR "message too long\n");
		return -1;
	}

	if (0) {
		printk(KERN_INFO "write: ");
		for (i=0; i<len; i++) {
			printk("%02x ", (buf[i] & 0xff));
		}
		printk("\n");
	}

	ptr = out;
	*ptr++ = 0xAA; /* sync */
	*ptr++ = len * 2; /* length */
	
	/* data*/
	for (i=0; i<len; i++) {
		checksum += buf[i];
		*ptr++ = int_to_hex((buf[i] >> 4) & 0x0F);
		*ptr++ = int_to_hex(buf[i] & 0x0F);
	}

	/* checksum */
	*ptr++ = int_to_hex((checksum >> 4) & 0x0F);
	*ptr++ = int_to_hex(checksum & 0x0F);

	return tty->driver->write(tty, out, out_len);
}


/* parse a message from the pic */
static void pic24f_tty_rx_parse(struct tty_struct *tty, const unsigned char *cp,
				char *fp, int count)
{
	struct pic24f_device *pic24f = (struct pic24f_device *)tty->disc_data;

	while (count--) {
		unsigned char c;

		if (fp && *fp++) {
			unsigned char buf[1];

			/* send NACK on parity error */
			buf[0] = PIC_CMD_NACK;
			pic24f_tty_tx(tty, buf, 1);

			pic24f->parser_state = 0;
			cp++;
			continue;
		}

		c = *cp++;
		
		if (c == 0xAA) {
			pic24f->parser_state = 0;
		}

		switch (pic24f->parser_state) {
		case 0: /* sync */
			if (c != 0xAA) {
				continue;
			}
			pic24f->rx_ptr = 0;
			pic24f->checksum = 0;
			pic24f->parser_state = 1;
			break;
			
		case 1: /* len */
			pic24f->rx_len = c / 2;
			if (pic24f->rx_len > PIC_MAX_LEN) {
				printk(KERN_INFO "msg too long %d\n", pic24f->rx_len);
				pic24f->parser_state = 0;
				continue;
			}
			pic24f->parser_state = 2;
			break;

		case 2: /* high data nibble */
			pic24f->rx_buf[pic24f->rx_ptr] = hex_to_int(c) << 4;
			pic24f->parser_state = 3;
			break;

		case 3: /* low data nibble */
			pic24f->rx_buf[pic24f->rx_ptr] |= hex_to_int(c);
			pic24f->checksum += pic24f->rx_buf[pic24f->rx_ptr++];
			pic24f->parser_state = (pic24f->rx_ptr < pic24f->rx_len) ? 2 : 4;
			break;

		case 4: /* low data nibble */
			pic24f->rx_checksum = hex_to_int(c) << 4;
			pic24f->parser_state = 5;
			break;

		case 5: /* low data nibble */
			pic24f->rx_checksum |= hex_to_int(c);
			pic24f->parser_state = 0;

			if (pic24f->rx_checksum != pic24f->checksum) {
				printk(KERN_INFO "checksum failed %d != %d\n", pic24f->rx_checksum, pic24f->checksum);

				pic24f->parser_state = 0;
				continue;
			}

			pic24f_msg_process(pic24f);
		}
	}
}


/* queue ircap data from pic */
static void pic24f_tty_rx_ircap(struct tty_struct *tty, const unsigned char *cp,
				char *fp, int count)
{
	size_t free;

	// TODO this ignores parity errors

	if (down_interruptible(&pic24f_device.ircap_lock)) {
		goto err;
	}

	while (count) {
		if (pic24f_device.ircap_rp == pic24f_device.ircap_wp) {
			free = IRCAP_BUF_SIZE;
		}
		else {
			free = ((pic24f_device.ircap_rp + IRCAP_BUF_SIZE - pic24f_device.ircap_wp) % IRCAP_BUF_SIZE) - 1;
		}

		if (!free) {
			goto err;
		}

		free = min((size_t)count, free);

		if (pic24f_device.ircap_wp >= pic24f_device.ircap_rp) {
			free = min(free, (size_t)(pic24f_device.ircap_buf + IRCAP_BUF_SIZE - pic24f_device.ircap_wp));
		}
		else {
			free = min(free, (size_t)(pic24f_device.ircap_rp - pic24f_device.ircap_wp -1));
		}

		memcpy(pic24f_device.ircap_wp, cp, free);

		count -= free;
		pic24f_device.ircap_wp += free;
		if (pic24f_device.ircap_wp == pic24f_device.ircap_buf + IRCAP_BUF_SIZE) {
			pic24f_device.ircap_wp = pic24f_device.ircap_buf;
		}
	}

	wake_up_interruptible(&pic24f_device.ircap_wait);

	up(&pic24f_device.ircap_lock);
	return;

err:
	up(&pic24f_device.ircap_lock);
	printk(KERN_ERR "IRCAP dropped %d bytes\n", count);
}


/* process serial data from the pic */
static void pic24f_tty_receive_buf(struct tty_struct *tty, const unsigned char *cp,
				   char *fp, int count)
{
	if (pic24f_device.ircap) {
		pic24f_tty_rx_ircap(tty, cp, fp, count);
	}
	else {
		pic24f_tty_rx_parse(tty, cp, fp, count);
	}
}

/* queue irlisten data from pic */
static void irlisten_rx_process (const unsigned char *cp, int count)
{
	size_t free;

	if (down_interruptible(&pic24f_device.irlisten_lock)) {
		goto err;
	}

	while (count) {
		if (pic24f_device.irlisten_rp == pic24f_device.irlisten_wp) {
			free = IRLISTEN_BUF_SIZE;
		}
		else {
			free = ((pic24f_device.irlisten_rp + IRLISTEN_BUF_SIZE - pic24f_device.irlisten_wp) % IRLISTEN_BUF_SIZE) - 1;
		}

		if (!free) {
			goto err;
		}

		free = min((size_t)count, free);

		if (pic24f_device.irlisten_wp >= pic24f_device.irlisten_rp) {
			free = min(free, (size_t)(pic24f_device.irlisten_buf + IRLISTEN_BUF_SIZE - pic24f_device.irlisten_wp));
		} else {
			free = min(free, (size_t)(pic24f_device.irlisten_rp - pic24f_device.irlisten_wp -1));
		}

		memcpy(pic24f_device.irlisten_wp, cp, free);

		count -= free;
		pic24f_device.irlisten_wp += free;
		if (pic24f_device.irlisten_wp == pic24f_device.irlisten_buf + IRLISTEN_BUF_SIZE) {
			pic24f_device.irlisten_wp = pic24f_device.irlisten_buf;
		}
	}

	wake_up_interruptible(&pic24f_device.irlisten_wait);

	up(&pic24f_device.irlisten_lock);
	return;

err:
	up(&pic24f_device.irlisten_lock);
	printk(KERN_ERR "IRLISTEN data dropped %d bytes\n", count);
}

/* set serial baud rate */
static void set_baud(struct tty_struct *tty, unsigned int baudcode)
{
	struct ktermios old_termios = *(tty->termios);
	tty->termios->c_cflag &= ~CBAUD;	/* Clear the old baud setting */
	tty->termios->c_cflag |= baudcode;	/* Set the new baud setting */
	tty->driver->set_termios(tty, &old_termios);
}


/* open our tty line line discipline */
static int pic24f_tty_open(struct tty_struct *tty)
{
	tty->disc_data = &pic24f_device;
	pic24f_device.tty = tty;

	set_baud(tty, B115200);

	// TODO other init, maybe check firmware version?

	return 0;
}


/* close our tty line line discipline */
static void pic24f_tty_close(struct tty_struct *tty)
{
	pic24f_device.tty = NULL;
}


static struct tty_ldisc pic24f_ldisc = {
	.magic = TTY_LDISC_MAGIC,
	.name = "pic24f",
	.owner = THIS_MODULE,
	.open = pic24f_tty_open,
	.close = pic24f_tty_close,
	.receive_buf = pic24f_tty_receive_buf,
};


/* send a message to the pic, handle locking and retries */
static int pic24f_msg_tx(struct pic24f_device *pic24f, bool unlock, const unsigned char *buf, size_t len)
{
	int r, tries = 5;

	if (pic24f->tty == NULL) {
		return -EIO;
	}

	/* aquire lock */
	if (down_interruptible(&pic24f->lock)) {
		return -ERESTARTSYS;
	}

	while (tries--) {
		pic24f->acknack = 0;
		if ((r = pic24f_tty_tx(pic24f->tty, buf, len)) < 0) {
			goto err;
		}

		/* wait for ack/nack */
		// TODO check intel spec for correct timeout value
		if ((r = wait_event_interruptible_timeout(pic24f->wait, pic24f->acknack, TIMEOUT)) < 0) {
			goto err;
		}
		
		if (pic24f->acknack == PIC_CMD_ACK) {
			break;
		}

		printk(KERN_INFO "pic24f: timeout waiting for ACK (cmd %02x)\n", buf[0]);
	}

	if (!tries) {
		r = EIO;
		goto err;
	}

	/* optionally release lock (so ir cap/gen can hold lock) */
	if (unlock) {
		up(&pic24f->lock);
	}
	return 0;

 err:
	up(&pic24f->lock);
	return r;
}


/* process message received from pic */
static void pic24f_msg_process(struct pic24f_device *pic24f)
{
	int n;
	unsigned char cmd, buf[1];

	if (0) {
		int i;

		printk(KERN_INFO "read: ");
		for (i=0; i<pic24f->rx_len; i++) {
			printk("%02x ", pic24f->rx_buf[i]);
		}
		printk("\n");
	}

	cmd = pic24f->rx_buf[0];

	switch (cmd) {
	case PIC_CMD_ACK:
	case PIC_CMD_NACK:
		pic24f->acknack = cmd;
		wake_up_interruptible(&pic24f->wait);
		return;

	case PIC_CMD_VERSION:
		n = pic24f->rx_buf[1];
		memcpy(pic24f->version, pic24f->rx_buf + 2, n);
		pic24f->version[n+1] = 0;
		wake_up_interruptible(&pic24f->wait);
		break;

	case PIC_CMD_GIGABYTE:
		switch (pic24f->rx_buf[1]) {
		case PIC_CMD_FAN_CTRL_GET:
			if (pic24f->rx_buf[2] == '1') {
				sscanf(pic24f->rx_buf+3, "%d", &pic24f->cputemp);
				wake_up_interruptible(&pic24f->wait);
			}
			else if (pic24f->rx_buf[2] == '2') {
				sscanf(pic24f->rx_buf+3, "%d", &pic24f->fanrpm);
				wake_up_interruptible(&pic24f->wait);
			}
		}
		break;

	case PIC_CMD_IRTX_FINISHED:
		pic24f->irtx_finished = pic24f->rx_buf[1] - '0';
		wake_up_interruptible(&pic24f->wait);
		break;

	case PIC_CMD_BUTTON:
		input_event(pic24f->input_dev, EV_KEY, KEY_START_PAIRIING, (pic24f->rx_buf[1] == 1));
		input_sync(pic24f->input_dev);
		break;

	case PIC_CMD_LISTEN_DATA:
		n = pic24f->rx_buf[1];
		irlisten_rx_process(pic24f->rx_buf + 2, n);
		break;

	// TODO CEC

	default:
		printk(KERN_ERR "pic24f: unknown message %02x\n", cmd);
	}

	buf[0] = PIC_CMD_ACK;
	pic24f_tty_tx(pic24f->tty, buf, 1);
}


static ssize_t pic24f_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r;
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_VERSION,
	};

	*pic24f_device.version = '\0';
	if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 1)) < 0) {
		return r;
	}

	if ((r = wait_event_interruptible_timeout(pic24f_device.wait, *pic24f_device.version, TIMEOUT)) < 0) {
		return r;
	}
	if (!*pic24f_device.version) {
		return -ETIME;
	}

	return sprintf(buf, "%s\n", pic24f_device.version);
}

DEVICE_ATTR(version, S_IRUGO, &pic24f_version, NULL);


static ssize_t pic24f_cputemp(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r;
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_FAN_CTRL_GET,
		'1',
	};

	pic24f_device.cputemp = -1;
	if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 3)) < 0) {
		return r;
	}

	if ((r = wait_event_interruptible_timeout(pic24f_device.wait, pic24f_device.cputemp >= 0, TIMEOUT)) < 0) {
		return r;
	}
	if (pic24f_device.cputemp < 0) {
		return -ETIME;
	}

	return sprintf(buf, "%d\n", pic24f_device.cputemp);
}

DEVICE_ATTR(cputemp, S_IRUGO, &pic24f_cputemp, NULL);


static ssize_t pic24f_fanrpm(struct device *dev, struct device_attribute *attr, char *buf)
{
	int r;
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_FAN_CTRL_GET,
		'2',
	};

	pic24f_device.fanrpm = -1;
	if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 3)) < 0) {
		return r;
	}

	if ((r = wait_event_interruptible_timeout(pic24f_device.wait, pic24f_device.fanrpm >= 0, TIMEOUT)) < 0) {
		return r;
	}
	if (pic24f_device.fanrpm < 0) {
		return -ETIME;
	}

	return sprintf(buf, "%d\n", pic24f_device.fanrpm);
}

DEVICE_ATTR(fanrpm, S_IRUGO, &pic24f_fanrpm, NULL);


static ssize_t pic24f_led1(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int state, r;
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_LED_CON,
		70,
		0
	};

	sscanf(buf, "%d", &state);

	switch (state) {
	default:
	case 0: /* off */
		msg[3] = 123 - 70;
		break;
	case 1: /* on */
		msg[3] = 120 - 70;
		break;
	case 2: /* blink */
		msg[3] = 121 - 70;
		break;
	}

	if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 4)) < 0) {
		return r;
	}

	return count;
}

DEVICE_ATTR(led1, S_IWUSR, NULL, &pic24f_led1);


static ssize_t pic24f_led2(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int state, r;
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_LED_CON,
		70,
		0
	};

	sscanf(buf, "%d", &state);

	switch (state) {
	default:
	case 0: /* off */
		msg[3] = 133 - 70;
		break;
	case 1: /* on */
		msg[3] = 131 - 70;
		break;
	case 2: /* blink */
		msg[3] = 132 - 70;
		break;
	}

	if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 4)) < 0) {
		return r;
	}

	return count;
}

DEVICE_ATTR(led2, S_IWUSR, NULL, &pic24f_led2);


static ssize_t pic24f_program(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	// TODO
	return count;
}

DEVICE_ATTR(program, S_IWUSR, NULL, &pic24f_program);


static struct attribute *pic24f_attributes[] = {
	&dev_attr_version.attr,
	&dev_attr_cputemp.attr,
	&dev_attr_fanrpm.attr,
	&dev_attr_led1.attr,
	&dev_attr_led2.attr,
	&dev_attr_program.attr,
	NULL
};

static const struct attribute_group pic24f_attr_group = {
	.attrs = pic24f_attributes,
};


/*
 * IR
 */

int pic24f_ir_open(struct inode *inode, struct file *file)
{
	if (pic24f_device.tty == NULL) {
		return -EIO;
	}

	return 0;
}

ssize_t pic24f_ir_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	int r;
	unsigned char wbuf[count];
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_IRTX_START,
	};

	/* NOTE: This assumes the write contains a single IR packet */
	// TODO maybe parse the data to ensure single packets are sent

	if (copy_from_user(wbuf, buf, count)) {
		return -EFAULT;
	}

	/* send IR packet start message to PIC, leaving tty locked */
	msg[1] = (count >> 8) & 0xFF;
	msg[2] = count & 0xFF;

	if ((r = pic24f_msg_tx(&pic24f_device, false, msg, 3)) < 0) {
		return r;
	}

	/* send IR data, and unlock tty */
	pic24f_device.tty->driver->write(pic24f_device.tty, wbuf, count);
	up(&pic24f_device.lock);

	return 0;
}

int pic24f_ir_ioctl(struct inode *inode, struct file *file,
		    unsigned int cmd, unsigned long arg)
{
	unsigned char msg[PIC_MAX_LEN];
	int r;

	if (_IOC_TYPE(cmd) != PIC24_IOC_MAGIC) {
		return -EINVAL;
	}
	if (_IOC_NR(cmd) > PIC24_IOC_MAXNR) {
		return -EINVAL;
	}

	switch (cmd) {
	case PIC24_IOCIRGENSTOP:
		/* send IR stop */
		msg[0] = PIC_CMD_IRTX_STOP;
		return pic24f_msg_tx(&pic24f_device, true, msg, 1);

	case PIC24_IOCIRGENFINISHED:
		/* send IR stop */
		do {
			pic24f_device.irtx_finished = -1;

			msg[0] = PIC_CMD_IRTX_FINISHED;
			if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 1)) < 0) {
				return r;
			}

			if ((r = wait_event_interruptible_timeout(pic24f_device.wait, pic24f_device.irtx_finished >= 0, TIMEOUT)) < 0) {
				return r;
			}
			if (pic24f_device.irtx_finished < 0) {
				return -ETIME;
			}

			udelay(10);
		} while (pic24f_device.irtx_finished != 0);

		return 0;
	
	case PIC24_IOCIRBEAT:
		/* send IR stop */
		msg[0] = PIC_CMD_IR_BEAT;
		return pic24f_msg_tx(&pic24f_device, true, msg, 1);

	default:
		return -EINVAL;
	}
}

static const struct file_operations pic24f_ir_fops = {
	.owner		= THIS_MODULE,
	.open		= pic24f_ir_open,
	.write		= pic24f_ir_write,
	.ioctl		= pic24f_ir_ioctl,
};


/*
 * IR Capture
 */

int pic24f_ircap_open(struct inode *inode, struct file *file)
{
	if (pic24f_device.tty == NULL) {
		return -EIO;
	}

	pic24f_device.ircap_buf = kmalloc(IRCAP_BUF_SIZE, GFP_KERNEL);
	pic24f_device.ircap_rp = pic24f_device.ircap_wp = pic24f_device.ircap_buf;

	if (!pic24f_device.ircap_buf) {
		return -ENOMEM;
	}

	// TODO disable watchdog ?

	return 0;
}

ssize_t pic24f_ircap_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	if (down_interruptible(&pic24f_device.ircap_lock)) {
		return -ERESTARTSYS;
	}

	while (pic24f_device.ircap_rp == pic24f_device.ircap_wp) {
		/* nothing to read */
		up(&pic24f_device.ircap_lock);

		// TODO support non-block?

		if (wait_event_interruptible(pic24f_device.ircap_wait, (pic24f_device.ircap_rp != pic24f_device.ircap_wp))) {
			return -ERESTARTSYS;
		}
		if (down_interruptible(&pic24f_device.ircap_lock)) {
			return -ERESTARTSYS;
		}
	}

	/* return bytes */
	if (pic24f_device.ircap_wp > pic24f_device.ircap_rp) {
		count = min(count, (size_t)(pic24f_device.ircap_wp - pic24f_device.ircap_rp));
	}
	else {
		count = min(count, (size_t)(pic24f_device.ircap_buf + IRCAP_BUF_SIZE - pic24f_device.ircap_rp));
	}

	if (copy_to_user(buf, pic24f_device.ircap_rp, count)) {
		up(&pic24f_device.ircap_lock);
		return -EFAULT;
	}

	pic24f_device.ircap_rp += count;
	if (pic24f_device.ircap_rp == pic24f_device.ircap_buf + IRCAP_BUF_SIZE) {
		pic24f_device.ircap_rp = pic24f_device.ircap_buf;
	}
	up(&pic24f_device.ircap_lock);

	return count;
}

unsigned int pic24f_ircap_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	down(&pic24f_device.ircap_lock);

	poll_wait(file, &pic24f_device.ircap_wait, wait);
	if (pic24f_device.ircap_rp != pic24f_device.ircap_wp) {
		mask |= POLLIN | POLLRDNORM;
	}

	up(&pic24f_device.ircap_lock);

	return mask;
}

int pic24f_ircap_release(struct inode *inode, struct file *file)
{
	/* restore rx parser to normal mode, and unlock tty */
	if (pic24f_device.ircap) {
		pic24f_device.ircap = false;
		up(&pic24f_device.lock);
	}

	// TODO enable watchdog ?

	kfree(pic24f_device.ircap_buf);

	return 0;
}

int pic24f_ircap_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	unsigned char msg[PIC_MAX_LEN];
	int r;

	if (_IOC_TYPE(cmd) != PIC24_IOC_MAGIC) {
		return -EINVAL;
	}
	if (_IOC_NR(cmd) > PIC24_IOC_MAXNR) {
		return -EINVAL;
	}

	switch (cmd) {
	case PIC24_IOCIRCAPSTART:
		msg[0] = PIC_CMD_IRCAP_START;

		/* send IR capture start message to PIC, leaving tty locked */
		if ((r = pic24f_msg_tx(&pic24f_device, false, msg, 4)) < 0) {
			return r;
		}

		/* enable ir capture mode */
		pic24f_device.ircap = true;

		return 0;

	case PIC24_IOCIRCAPSTOP:
		/* any character will cancel ircap mode */
		pic24f_device.tty->driver->write(pic24f_device.tty, msg, 1);
		
		/* the app can now read the remaining data before closing the device */
		return 0;

	default:
		return -EINVAL;
	}
}

static const struct file_operations pic24f_ircap_fops = {
	.owner		= THIS_MODULE,
	.open		= pic24f_ircap_open,
	.release	= pic24f_ircap_release,
	.read		= pic24f_ircap_read,
	.poll		= pic24f_ircap_poll,
	.ioctl		= pic24f_ircap_ioctl,
};


/*
 * IR Listen
 */
int pic24f_irlisten_open(struct inode *inode, struct file *file)
{
	if (pic24f_device.tty == NULL) {
		return -EIO;
	}

	pic24f_device.irlisten_buf = kmalloc(IRLISTEN_BUF_SIZE, GFP_KERNEL);
	pic24f_device.irlisten_rp = pic24f_device.irlisten_wp = pic24f_device.irlisten_buf;

	if (!pic24f_device.irlisten_buf) {
		return -ENOMEM;
	}
	return 0;
}

ssize_t pic24f_irlisten_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	if (down_interruptible(&pic24f_device.irlisten_lock)) {
		return -ERESTARTSYS;
	}

	while (pic24f_device.irlisten_rp == pic24f_device.irlisten_wp) {
		/* nothing to read */
		up(&pic24f_device.irlisten_lock);

		if (wait_event_interruptible(pic24f_device.irlisten_wait, (pic24f_device.irlisten_rp != pic24f_device.irlisten_wp))) {
			return -ERESTARTSYS;
		}
		if (down_interruptible(&pic24f_device.irlisten_lock)) {
			return -ERESTARTSYS;
		}
	}

	/* return bytes */
	if (pic24f_device.irlisten_wp > pic24f_device.irlisten_rp) {
		count = min(count, (size_t)(pic24f_device.irlisten_wp - pic24f_device.irlisten_rp));
	}
	else {
		count = min(count, (size_t)(pic24f_device.irlisten_buf + IRLISTEN_BUF_SIZE - pic24f_device.irlisten_rp));
	}

	if (copy_to_user(buf, pic24f_device.irlisten_rp, count)) {
		up(&pic24f_device.irlisten_lock);
		return -EFAULT;
	}

	pic24f_device.irlisten_rp += count;
	if (pic24f_device.irlisten_rp == pic24f_device.irlisten_buf + IRLISTEN_BUF_SIZE) {
		pic24f_device.irlisten_rp = pic24f_device.irlisten_buf;
	}
	up(&pic24f_device.irlisten_lock);

	return count;
}

unsigned int pic24f_irlisten_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	down(&pic24f_device.irlisten_lock);

	poll_wait(file, &pic24f_device.irlisten_wait, wait);
	if (pic24f_device.irlisten_rp != pic24f_device.irlisten_wp) {
		mask |= POLLIN | POLLRDNORM;
	}

	up(&pic24f_device.irlisten_lock);

	return mask;
}

int pic24f_irlisten_release(struct inode *inode, struct file *file)
{
	kfree(pic24f_device.irlisten_buf);

	return 0;
}

int pic24f_irlisten_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	unsigned char msg[PIC_MAX_LEN];
	int r;

	if (_IOC_TYPE(cmd) != PIC24_IOC_MAGIC) {
		return -EINVAL;
	}
	if (_IOC_NR(cmd) > PIC24_IOC_MAXNR) {
		return -EINVAL;
	}

	switch (cmd) {

	case PIC24_IOCIRLISTENSTART:
		msg[0] = PIC_CMD_LISTEN_START;
		/* send IR listen start message to PIC, 
		   PIC will start sending IR listen packets to this driver
		   only after it recieves this command  */
		if ((r = pic24f_msg_tx(&pic24f_device, true, msg, 4)) < 0) {
			return r;
		}
		return 0;

	case PIC24_IOCIRLISTENSTOP:
		/* May be we never do a stop*/
		return 0;

	default:
		return -EINVAL;
	}
}

static const struct file_operations pic24f_irlisten_fops = {
	.owner		= THIS_MODULE,
	.open		= pic24f_irlisten_open,
	.release	= pic24f_irlisten_release,
	.read		= pic24f_irlisten_read,
	.poll		= pic24f_irlisten_poll,
	.ioctl		= pic24f_irlisten_ioctl,
};



/*
 * CEC
 */

// TODO cec device

static const struct file_operations pic24f_cec_fops = {
	.owner		= THIS_MODULE,
	.read		= NULL,
	.write		= NULL,
};



/*
 * Watchdog
 *
 * Based on watchdog/softdog.c
 */
static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static unsigned long driver_open, orphan_timer;
static char expect_close;


static int pic24f_wdog_start(void)
{
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_WDT,
		'E',
		'N',
	};

	return pic24f_msg_tx(&pic24f_device, true, msg, 4);
}

static int pic24f_wdog_keepalive(void)
{
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_WDT,
		'C',
		'L',
		'R',
	};

	return pic24f_msg_tx(&pic24f_device, true, msg, 5);
}

static int pic24f_wdog_stop(void)
{
	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_WDT,
		'D',
		'I',
		'S',
	};

	return pic24f_msg_tx(&pic24f_device, true, msg, 5);
}

static int pic24f_wdog_set_heartbeat(int t)
{
  	unsigned char msg[PIC_MAX_LEN] = {
		PIC_CMD_GIGABYTE,
		PIC_CMD_WDT,
		'S',
		'E',
		'T',
		'P',
	};

 	if ((t < 0x0001) || (t > 0xFFFF))
 		return -EINVAL;
 
    msg[6] = (t & 0xff00) >> 8;
    msg[7] = t & 0xff;

	return pic24f_msg_tx(&pic24f_device, true, msg, 8);

	return 0;
}

static int pic24f_wdog_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &driver_open))
		return -EBUSY;
	if (!test_and_clear_bit(0, &orphan_timer))
		__module_get(THIS_MODULE);

	if (pic24f_device.tty == NULL) {
		return -EIO;
	}

	/* Activate timer */
	pic24f_wdog_start();
	return nonseekable_open(inode, file);
}

static int pic24f_wdog_release(struct inode *inode, struct file *file)
{
	/* Shut off the timer.
	 * Lock it in if it's a module and we set nowayout
	 */
	if (expect_close == 42) {
		pic24f_wdog_stop();
		module_put(THIS_MODULE);
	} else {
		printk(KERN_CRIT "Unexpected close, not stopping watchdog!\n");
		set_bit(0, &orphan_timer);
		pic24f_wdog_keepalive();
	}
	clear_bit(0, &driver_open);
	expect_close = 0;
	return 0;
}

static ssize_t pic24f_wdog_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	/* Refresh the timer. */
	if (len) {
		if (!nowayout) {
			size_t i;

			/* In case it was set long ago */
			expect_close = 0;

			for (i = 0; i != len; i++) {
				char c;

				if (get_user(c, data + i))
					return -EFAULT;
				if (c == 'V')
					expect_close = 42;
			}
		}
		pic24f_wdog_keepalive();
	}
	return len;
}

static int pic24f_wdog_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int new_margin;
	static struct watchdog_info ident = {
		.options =		WDIOF_SETTIMEOUT |
					WDIOF_KEEPALIVEPING |
					WDIOF_MAGICCLOSE,
		.firmware_version =	0,
		.identity =		"Software Watchdog",
	};
	switch (cmd) {
		default:
			return -ENOTTY;
		case WDIOC_GETSUPPORT:
			return copy_to_user(argp, &ident,
				sizeof(ident)) ? -EFAULT : 0;
		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			// TODO return boot status
			return put_user(0, p);
		case WDIOC_KEEPALIVE:
			pic24f_wdog_keepalive();
			return 0;
		case WDIOC_SETTIMEOUT:
			if (get_user(new_margin, p))
				return -EFAULT;
			if (pic24f_wdog_set_heartbeat(new_margin))
				return -EINVAL;
			pic24f_wdog_keepalive();
			/* Fall */
		case WDIOC_GETTIMEOUT:
			// TODO return heartbeat interval
			return put_user(0 /* TODO soft_margin*/, p);
	}
}

static int pic24f_wdog_notify_sys(struct notifier_block *this, unsigned long code,
	void *unused)
{
	if(code == SYS_DOWN || code == SYS_HALT) {
		/* Turn the WDT off */
		pic24f_wdog_stop();
	}
	return NOTIFY_DONE;
}

static const struct file_operations pic24f_wdog_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= pic24f_wdog_write,
	.ioctl		= pic24f_wdog_ioctl,
	.open		= pic24f_wdog_open,
	.release	= pic24f_wdog_release,
};

static struct notifier_block pic24f_dog_notifier = {
	.notifier_call	= pic24f_wdog_notify_sys,
};


/*
 * Module init
 */
static struct pic24f_device pic24f_device = {
	.ir_device	= {
		.minor		= MISC_DYNAMIC_MINOR,
		.name		= "ir",
		.fops		= &pic24f_ir_fops,
	},
	.ircap_device	= {
		.minor		= MISC_DYNAMIC_MINOR,
		.name		= "ircap",
		.fops		= &pic24f_ircap_fops,
	},
	.cec_device	= {
		.minor		= MISC_DYNAMIC_MINOR,
		.name		= "cec",
		.fops		= &pic24f_cec_fops,
	},
	.irlisten_device	= {
		.minor		= MISC_DYNAMIC_MINOR,
		.name		= "irlisten",
		.fops		= &pic24f_irlisten_fops,
	},
	.wdog_device    = {
		.minor		= WATCHDOG_MINOR,
		.name		= "watchdog",
		.fops		= &pic24f_wdog_fops,
	},
};


static int __init logitech_ka_pic_init(void)
{
	int ret;

	init_MUTEX(&pic24f_device.lock);
	init_MUTEX(&pic24f_device.ircap_lock);
	init_MUTEX(&pic24f_device.irlisten_lock);
	init_waitqueue_head(&pic24f_device.wait);
	init_waitqueue_head(&pic24f_device.ircap_wait);
	init_waitqueue_head(&pic24f_device.irlisten_wait);

	ret = tty_register_ldisc(N_PIC24F, &pic24f_ldisc);
	if (ret) {
		printk(KERN_ERR "%s:%u: can't register line discipline %d\n",  __func__, __LINE__, ret);
		goto err0;
	}

	ret = register_reboot_notifier(&pic24f_dog_notifier);
	if (ret) {
		printk(KERN_ERR "%s:%u: register_reboot_notifier failed %d\n", __func__, __LINE__, ret);
		goto err1;
	}

	ret = misc_register(&pic24f_device.ir_device);
	if (ret) {
		printk(KERN_ERR "%s:%u: misc_register failed %d\n", __func__, __LINE__, ret);
		goto err2;
	}

	ret = misc_register(&pic24f_device.ircap_device);
	if (ret) {
		printk(KERN_ERR "%s:%u: misc_register failed %d\n", __func__, __LINE__, ret);
		goto err3;
	}

	ret = misc_register(&pic24f_device.cec_device);
	if (ret) {
		printk(KERN_ERR "%s:%u: misc_register failed %d\n", __func__, __LINE__, ret);
		goto err4;
	}

	ret = misc_register(&pic24f_device.wdog_device);
	if (ret) {
		printk(KERN_ERR "%s:%u: misc_register failed %d\n", __func__, __LINE__, ret);
		goto err5;
	}

	ret = misc_register(&pic24f_device.irlisten_device);
	if (ret) {
		printk(KERN_ERR "%s:%u: misc_register failed %d\n", __func__, __LINE__, ret);
		goto err6;
	}

	pic24f_device.input_dev = input_allocate_device();
	if (!pic24f_device.input_dev) {
		printk(KERN_ERR "%s:%u: input_allocate_device failed %d\n", __func__, __LINE__, ret);
		goto err7;
	}

	pic24f_device.input_dev->name = "Logitech KA";
	input_set_capability(pic24f_device.input_dev, EV_KEY, KEY_START_PAIRIING);
	
	//Virgilio: this is needed to let Android:EventHub to let this input device to pass as a kbd device
	// and do all the magic needed to map the KEY_START_PAIRING scancode to the proper keycode
	input_set_capability(pic24f_device.input_dev, EV_KEY, KEY_Q);

	ret = input_register_device(pic24f_device.input_dev);
	if (ret) {
		printk(KERN_ERR "%s:%u: input_register_device failed %d\n", __func__, __LINE__, ret);
		goto err7;
	}


	// TODO better place for /sys interface?
	ret = sysfs_create_group(&pic24f_device.ir_device.this_device->kobj, &pic24f_attr_group);
	if (ret) {
		printk(KERN_ERR "%s:%u: sysfs_create_group failed %d\n", __func__, __LINE__, ret);
		goto err8;
	}

	return 0;

 err8:
	input_unregister_device(pic24f_device.input_dev);
 err7:
	misc_deregister(&pic24f_device.irlisten_device);
 err6:
	misc_deregister(&pic24f_device.wdog_device);
 err5:
	misc_deregister(&pic24f_device.cec_device);
 err4:
	misc_deregister(&pic24f_device.ircap_device);
 err3:
	misc_deregister(&pic24f_device.ir_device);
 err2:
	unregister_reboot_notifier(&pic24f_dog_notifier);
 err1:
	tty_unregister_ldisc(N_PIC24F);
 err0:
	return ret;
}


static void __exit logitech_ka_pic_exit(void)
{
	sysfs_remove_group(&pic24f_device.ir_device.this_device->kobj, &pic24f_attr_group);
	input_unregister_device(pic24f_device.input_dev);
	misc_deregister(&pic24f_device.irlisten_device);
	misc_deregister(&pic24f_device.wdog_device);
	misc_deregister(&pic24f_device.cec_device);
	misc_deregister(&pic24f_device.ircap_device);
	misc_deregister(&pic24f_device.ir_device);
	unregister_reboot_notifier(&pic24f_dog_notifier);
	tty_unregister_ldisc(N_PIC24F);
}


module_init(logitech_ka_pic_init);
module_exit(logitech_ka_pic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("richard_titmuss@logitech.com");
