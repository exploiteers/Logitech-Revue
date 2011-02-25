/*
 * Freescale QUICC Engine USB Host Controller Driver
 *
 * Copyright (c) Freescale Semicondutor, Inc. 2006.
 *               Shlomi Gridish <gridish@freescale.com>
 *               Jerry Huang <Chang-Ming.Huang@freescale.com>
 * Copyright (c) Logic Product Development, Inc. 2007
 *               Peter Barada <peterb@logicpd.com>
 * Copyright (c) MontaVista Software, Inc. 2008.
 *               Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#define DUMMY_BD_BUFFER  0xdeadbeef
#define DUMMY2_BD_BUFFER 0xbaadf00d

/* transmit BD's status*/
#define T_R		0x80000000	/* ready bit */
#define T_W		0x20000000	/* wrap bit */
#define T_I		0x10000000	/* interrupt on completion */
#define T_L		0x08000000	/* last */
#define T_TC		0x04000000	/* transmit CRC */
#define T_CNF		0x02000000	/* ucode wait for  transmit confirm
					 *  after this BD */
#define T_LSP		0x01000000	/* Low-speed transaction */
#define T_PID		0x00c00000	/* packet id */
#define T_NAK		0x00100000	/* No ack. */
#define T_STAL		0x00080000	/* Stall recieved */
#define T_TO		0x00040000	/* time out */
#define T_UN		0x00020000	/* underrun */

#define DEVICE_T_ERROR		(T_UN | T_TO)
#define HOST_T_ERROR		(T_UN | T_TO | T_NAK | T_STAL)
#define DEVICE_T_BD_MASK	DEVICE_T_ERROR
#define HOST_T_BD_MASK		HOST_T_ERROR

#define T_PID_SHIFT	6
#define T_PID_DATA0	0x00800000	/* Data 0 toggle */
#define T_PID_DATA1	0x00c00000	/* Data 1 toggle */

/* receive BD's status */
#define R_E		0x80000000	/* buffer empty */
#define R_W		0x20000000	/* wrap bit */
#define R_I		0x10000000	/* interrupt on reception */
#define R_L		0x08000000	/* last */
#define R_F		0x04000000	/* first */
#define R_PID		0x00c00000	/* packet id */
#define R_NO		0x00100000	/* Rx Non Octet Aligned Packet */
#define R_AB		0x00080000	/* Frame Aborted */
#define R_CR		0x00040000	/* CRC Error */
#define R_OV		0x00020000	/* Overrun */

#define R_ERROR		(R_NO | R_AB | R_CR | R_OV)
#define R_BD_MASK	R_ERROR

#define R_PID_DATA0	0x00000000
#define R_PID_DATA1	0x00400000
#define R_PID_SETUP	0x00800000

struct usb_bd {
	u16 status;		/*volatile */
	u16 length;
	u32 buf;
} __attribute__ ((packed));
#define SIZEOF_USB_BD  8

#define USB_BD_LENGTH_MASK 0x0000ffff
#define USB_BD_STATUS_MASK 0xffff0000
#define USB_BD_BUFFER_ARG(bd)		((struct usb_bd*)bd)->buf
#define USB_BD_BUFFER_CLEAR(bd)		out_be32(&(BD_BUFFER_ARG(bd)),0)
#define USB_BD_BUFFER(bd)		in_be32(&(BD_BUFFER_ARG(bd)))
#define USB_BD_STATUS_AND_LENGTH_SET(bd,val)	out_be32((u32*)bd,val)
#define USB_BD_STATUS_AND_LENGTH(bd)		in_be32((u32*)bd)
#define USB_BD_BUFFER_SET(bd,buffer)		out_be32(&(BD_BUFFER_ARG(bd)),\
							(u32)(buffer))

#define IN_TOKEN	0x69	/*IN token bit pid format according to USB1.1 */
#define OUT_TOKEN	0xe1	/*OUT token bit pid format according to USB1.1 */
#define SETUP_TOKEN	0x2d	/*SETUP token bit pid format according to USB1.1 */
#define STALL_TOKEN	0x1e	/*STALL token bit pid format according to USB1.1 */
#define NACK_TOKEN	0x5a	/*NACK token pid bit format according to USB1.1 */

#define ISO_THRESHOLD	0
#define OTHER_THRESHOLD	1

typedef enum {
	e_TRANSACTION_IDLE = 0,
	e_TRANSACTION_WAIT_FOR_TOKEN_CONF,
	e_TRANSACTION_WAIT_FOR_DATA_CONF,
	e_TRANSACTION_DATA_ALLREADY_CONF
} e_transaction_status;

typedef enum {
	e_HOST_TRANS_FREE,
	e_HOST_TRANS_BUSY,
	e_HOST_TRANS_WAIT
} e_host_transaction_status;

struct token {
	/* According to the USB 1.1 spec, each token must comply to 
	 * this format/structure.
	 */
	struct {
		u8 pid;		/* IN/OUT/SETUP token id */
		u16 pkt;	/* device address + endpoint address + crc5 */
	} __attribute__ ((packed)) token_packet;
	struct list_head node;
};

struct transaction {
	struct packet *token_frame;	/* pointer to associated token frame */
	struct packet *data_frame;	/* pointer to associated data frame */
	struct token *token;	/* pointer to associated token obj */
	struct usb_bd *token_bd;	/* pointer to associated token BD */
	struct usb_bd *data_bd;	/* pointer to associated data BD */

	e_transaction_status transaction_status;
	enum fhci_tf_mode mode;	/* transaction mode-ISO\INTR\BULK\CTRL */
	enum fhci_speed speed;	/* transaction speed - Slow\Fast */
	e_usb_trans_type type;	/* transaction type - IN\OUT\SETUP */
	u8 pid;			/* PID field - 0\1 */
	struct list_head node;	/* a node in a list of transactions */
};

struct host_transaction_context {
	void *usb;
	struct packet *user_frame;
	e_usb_trans_type transaction_type;
	u8 dest_addr;
	u8 dest_ep;
	enum fhci_tf_mode dest_trans_mode;
	enum fhci_speed dest_speed;
	u8 data_toggle;
	e_host_transaction_status curr_trans_status;
	enum fhci_tf_mode curr_trans_mode;
	u8 threshold;
};
struct endpoint {
	struct fhci_ep_pram *ep_pram_ptr;	/* Pointer to ep parameter RAM */

	/* Host transactions */
	struct list_head empty_tokens_list;	/* empty tokens list */
	struct list_head empty_transactions_list;	/* empty transactions list */
	struct list_head active_transactions_list;	/* progress transactions list */
	struct transaction *conf_transaction;	/* transaction to be confirmed */
	struct work_struct host_transaction_bh;	/* runs as a Bottom-Half */

	/* Transmit */
	struct usb_bd *tx_base;	/* first BD in Tx BD table */
	struct usb_bd *tx_conf_bd;	/* next BD for confirm after Tx */
	struct usb_bd *tx_empty_bd;	/* next BD for new Tx request */
	u8 data_0_1;		/* PID field - 0 or 1 */
	void *empty_frame_Q;	/* Empty frames list to use */
	void *conf_frame_Q;	/* frame passed to chip waiting for tx */
	void *dummy_packets_Q;	/* dummy packets for the CRC overun */

	/* Recieve */
	struct usb_bd *rx_base;	/* first BD in Rx BD table */
	struct usb_bd *rx_next_bd;	/* next BD to collect after Rx */
	struct usb_bd *rx_empty_bd;	/* next BD for new empty buffer */
	u32 bd_ring_len;	/* buffer number of the recieve */
	struct packet *pkt;	/* Static packet used for receive method */

	bool already_pushed_dummy_bd;
	bool was_busy_before_nacked;
	int nack_nesting_counter;
};
u16 token_pkts[0x80][0x10] = {
	{0x0010, 0x80A0, 0x0039, 0x8089, 0x0042, 0x80F2, 0x006B, 0x80DB,
	 0x00B4, 0x8004, 0x009D, 0x802D, 0x00E6, 0x8056, 0x00CF, 0x807F},
	{0x01E8, 0x8158, 0x01C1, 0x8171, 0x01BA, 0x810A, 0x0193, 0x8123,
	 0x014C, 0x81FC, 0x0165, 0x81D5, 0x011E, 0x81AE, 0x0137, 0x8187},
	{0x02A8, 0x8218, 0x0281, 0x8231, 0x02FA, 0x824A, 0x02D3, 0x8263,
	 0x020C, 0x82BC, 0x0225, 0x8295, 0x025E, 0x82EE, 0x0277, 0x82C7},
	{0x0350, 0x83E0, 0x0379, 0x83C9, 0x0302, 0x83B2, 0x032B, 0x839B,
	 0x03F4, 0x8344, 0x03DD, 0x836D, 0x03A6, 0x8316, 0x038F, 0x833F},
	{0x0428, 0x8498, 0x0401, 0x84B1, 0x047A, 0x84CA, 0x0453, 0x84E3,
	 0x048C, 0x843C, 0x04A5, 0x8415, 0x04DE, 0x846E, 0x04F7, 0x8447},
	{0x05D0, 0x8560, 0x05F9, 0x8549, 0x0582, 0x8532, 0x05AB, 0x851B,
	 0x0574, 0x85C4, 0x055D, 0x85ED, 0x0526, 0x8596, 0x050F, 0x85BF},
	{0x0690, 0x8620, 0x06B9, 0x8609, 0x06C2, 0x8672, 0x06EB, 0x865B,
	 0x0634, 0x8684, 0x061D, 0x86AD, 0x0666, 0x86D6, 0x064F, 0x86FF},
	{0x0768, 0x87D8, 0x0741, 0x87F1, 0x073A, 0x878A, 0x0713, 0x87A3,
	 0x07CC, 0x877C, 0x07E5, 0x8755, 0x079E, 0x872E, 0x07B7, 0x8707},
	{0x0860, 0x88D0, 0x0849, 0x88F9, 0x0832, 0x8882, 0x081B, 0x88AB,
	 0x08C4, 0x8874, 0x08ED, 0x885D, 0x0896, 0x8826, 0x08BF, 0x880F},
	{0x0998, 0x8928, 0x09B1, 0x8901, 0x09CA, 0x897A, 0x09E3, 0x8953,
	 0x093C, 0x898C, 0x0915, 0x89A5, 0x096E, 0x89DE, 0x0947, 0x89F7},
	{0x0AD8, 0x8A68, 0x0AF1, 0x8A41, 0x0A8A, 0x8A3A, 0x0AA3, 0x8A13,
	 0x0A7C, 0x8ACC, 0x0A55, 0x8AE5, 0x0A2E, 0x8A9E, 0x0A07, 0x8AB7},
	{0x0B20, 0x8B90, 0x0B09, 0x8BB9, 0x0B72, 0x8BC2, 0x0B5B, 0x8BEB,
	 0x0B84, 0x8B34, 0x0BAD, 0x8B1D, 0x0BD6, 0x8B66, 0x0BFF, 0x8B4F},
	{0x0C58, 0x8CE8, 0x0C71, 0x8CC1, 0x0C0A, 0x8CBA, 0x0C23, 0x8C93,
	 0x0CFC, 0x8C4C, 0x0CD5, 0x8C65, 0x0CAE, 0x8C1E, 0x0C87, 0x8C37},
	{0x0DA0, 0x8D10, 0x0D89, 0x8D39, 0x0DF2, 0x8D42, 0x0DDB, 0x8D6B,
	 0x0D04, 0x8DB4, 0x0D2D, 0x8D9D, 0x0D56, 0x8DE6, 0x0D7F, 0x8DCF},
	{0x0EE0, 0x8E50, 0x0EC9, 0x8E79, 0x0EB2, 0x8E02, 0x0E9B, 0x8E2B,
	 0x0E44, 0x8EF4, 0x0E6D, 0x8EDD, 0x0E16, 0x8EA6, 0x0E3F, 0x8E8F},
	{0x0F18, 0x8FA8, 0x0F31, 0x8F81, 0x0F4A, 0x8FFA, 0x0F63, 0x8FD3,
	 0x0FBC, 0x8F0C, 0x0F95, 0x8F25, 0x0FEE, 0x8F5E, 0x0FC7, 0x8F77},
	{0x10F0, 0x9040, 0x10D9, 0x9069, 0x10A2, 0x9012, 0x108B, 0x903B,
	 0x1054, 0x90E4, 0x107D, 0x90CD, 0x1006, 0x90B6, 0x102F, 0x909F},
	{0x1108, 0x91B8, 0x1121, 0x9191, 0x115A, 0x91EA, 0x1173, 0x91C3,
	 0x11AC, 0x911C, 0x1185, 0x9135, 0x11FE, 0x914E, 0x11D7, 0x9167},
	{0x1248, 0x92F8, 0x1261, 0x92D1, 0x121A, 0x92AA, 0x1233, 0x9283,
	 0x12EC, 0x925C, 0x12C5, 0x9275, 0x12BE, 0x920E, 0x1297, 0x9227},
	{0x13B0, 0x9300, 0x1399, 0x9329, 0x13E2, 0x9352, 0x13CB, 0x937B,
	 0x1314, 0x93A4, 0x133D, 0x938D, 0x1346, 0x93F6, 0x136F, 0x93DF},
	{0x14C8, 0x9478, 0x14E1, 0x9451, 0x149A, 0x942A, 0x14B3, 0x9403,
	 0x146C, 0x94DC, 0x1445, 0x94F5, 0x143E, 0x948E, 0x1417, 0x94A7},
	{0x1530, 0x9580, 0x1519, 0x95A9, 0x1562, 0x95D2, 0x154B, 0x95FB,
	 0x1594, 0x9524, 0x15BD, 0x950D, 0x15C6, 0x9576, 0x15EF, 0x955F},
	{0x1670, 0x96C0, 0x1659, 0x96E9, 0x1622, 0x9692, 0x160B, 0x96BB,
	 0x16D4, 0x9664, 0x16FD, 0x964D, 0x1686, 0x9636, 0x16AF, 0x961F},
	{0x1788, 0x9738, 0x17A1, 0x9711, 0x17DA, 0x976A, 0x17F3, 0x9743,
	 0x172C, 0x979C, 0x1705, 0x97B5, 0x177E, 0x97CE, 0x1757, 0x97E7},
	{0x1880, 0x9830, 0x18A9, 0x9819, 0x18D2, 0x9862, 0x18FB, 0x984B,
	 0x1824, 0x9894, 0x180D, 0x98BD, 0x1876, 0x98C6, 0x185F, 0x98EF},
	{0x1978, 0x99C8, 0x1951, 0x99E1, 0x192A, 0x999A, 0x1903, 0x99B3,
	 0x19DC, 0x996C, 0x19F5, 0x9945, 0x198E, 0x993E, 0x19A7, 0x9917},
	{0x1A38, 0x9A88, 0x1A11, 0x9AA1, 0x1A6A, 0x9ADA, 0x1A43, 0x9AF3,
	 0x1A9C, 0x9A2C, 0x1AB5, 0x9A05, 0x1ACE, 0x9A7E, 0x1AE7, 0x9A57},
	{0x1BC0, 0x9B70, 0x1BE9, 0x9B59, 0x1B92, 0x9B22, 0x1BBB, 0x9B0B,
	 0x1B64, 0x9BD4, 0x1B4D, 0x9BFD, 0x1B36, 0x9B86, 0x1B1F, 0x9BAF},
	{0x1CB8, 0x9C08, 0x1C91, 0x9C21, 0x1CEA, 0x9C5A, 0x1CC3, 0x9C73,
	 0x1C1C, 0x9CAC, 0x1C35, 0x9C85, 0x1C4E, 0x9CFE, 0x1C67, 0x9CD7},
	{0x1D40, 0x9DF0, 0x1D69, 0x9DD9, 0x1D12, 0x9DA2, 0x1D3B, 0x9D8B,
	 0x1DE4, 0x9D54, 0x1DCD, 0x9D7D, 0x1DB6, 0x9D06, 0x1D9F, 0x9D2F},
	{0x1E00, 0x9EB0, 0x1E29, 0x9E99, 0x1E52, 0x9EE2, 0x1E7B, 0x9ECB,
	 0x1EA4, 0x9E14, 0x1E8D, 0x9E3D, 0x1EF6, 0x9E46, 0x1EDF, 0x9E6F},
	{0x1FF8, 0x9F48, 0x1FD1, 0x9F61, 0x1FAA, 0x9F1A, 0x1F83, 0x9F33,
	 0x1F5C, 0x9FEC, 0x1F75, 0x9FC5, 0x1F0E, 0x9FBE, 0x1F27, 0x9F97},
	{0x2098, 0xA028, 0x20B1, 0xA001, 0x20CA, 0xA07A, 0x20E3, 0xA053,
	 0x203C, 0xA08C, 0x2015, 0xA0A5, 0x206E, 0xA0DE, 0x2047, 0xA0F7},
	{0x2160, 0xA1D0, 0x2149, 0xA1F9, 0x2132, 0xA182, 0x211B, 0xA1AB,
	 0x21C4, 0xA174, 0x21ED, 0xA15D, 0x2196, 0xA126, 0x21BF, 0xA10F},
	{0x2220, 0xA290, 0x2209, 0xA2B9, 0x2272, 0xA2C2, 0x225B, 0xA2EB,
	 0x2284, 0xA234, 0x22AD, 0xA21D, 0x22D6, 0xA266, 0x22FF, 0xA24F},
	{0x23D8, 0xA368, 0x23F1, 0xA341, 0x238A, 0xA33A, 0x23A3, 0xA313,
	 0x237C, 0xA3CC, 0x2355, 0xA3E5, 0x232E, 0xA39E, 0x2307, 0xA3B7},
	{0x24A0, 0xA410, 0x2489, 0xA439, 0x24F2, 0xA442, 0x24DB, 0xA46B,
	 0x2404, 0xA4B4, 0x242D, 0xA49D, 0x2456, 0xA4E6, 0x247F, 0xA4CF},
	{0x2558, 0xA5E8, 0x2571, 0xA5C1, 0x250A, 0xA5BA, 0x2523, 0xA593,
	 0x25FC, 0xA54C, 0x25D5, 0xA565, 0x25AE, 0xA51E, 0x2587, 0xA537},
	{0x2618, 0xA6A8, 0x2631, 0xA681, 0x264A, 0xA6FA, 0x2663, 0xA6D3,
	 0x26BC, 0xA60C, 0x2695, 0xA625, 0x26EE, 0xA65E, 0x26C7, 0xA677},
	{0x27E0, 0xA750, 0x27C9, 0xA779, 0x27B2, 0xA702, 0x279B, 0xA72B,
	 0x2744, 0xA7F4, 0x276D, 0xA7DD, 0x2716, 0xA7A6, 0x273F, 0xA78F},
	{0x28E8, 0xA858, 0x28C1, 0xA871, 0x28BA, 0xA80A, 0x2893, 0xA823,
	 0x284C, 0xA8FC, 0x2865, 0xA8D5, 0x281E, 0xA8AE, 0x2837, 0xA887},
	{0x2910, 0xA9A0, 0x2939, 0xA989, 0x2942, 0xA9F2, 0x296B, 0xA9DB,
	 0x29B4, 0xA904, 0x299D, 0xA92D, 0x29E6, 0xA956, 0x29CF, 0xA97F},
	{0x2A50, 0xAAE0, 0x2A79, 0xAAC9, 0x2A02, 0xAAB2, 0x2A2B, 0xAA9B,
	 0x2AF4, 0xAA44, 0x2ADD, 0xAA6D, 0x2AA6, 0xAA16, 0x2A8F, 0xAA3F},
	{0x2BA8, 0xAB18, 0x2B81, 0xAB31, 0x2BFA, 0xAB4A, 0x2BD3, 0xAB63,
	 0x2B0C, 0xABBC, 0x2B25, 0xAB95, 0x2B5E, 0xABEE, 0x2B77, 0xABC7},
	{0x2CD0, 0xAC60, 0x2CF9, 0xAC49, 0x2C82, 0xAC32, 0x2CAB, 0xAC1B,
	 0x2C74, 0xACC4, 0x2C5D, 0xACED, 0x2C26, 0xAC96, 0x2C0F, 0xACBF},
	{0x2D28, 0xAD98, 0x2D01, 0xADB1, 0x2D7A, 0xADCA, 0x2D53, 0xADE3,
	 0x2D8C, 0xAD3C, 0x2DA5, 0xAD15, 0x2DDE, 0xAD6E, 0x2DF7, 0xAD47},
	{0x2E68, 0xAED8, 0x2E41, 0xAEF1, 0x2E3A, 0xAE8A, 0x2E13, 0xAEA3,
	 0x2ECC, 0xAE7C, 0x2EE5, 0xAE55, 0x2E9E, 0xAE2E, 0x2EB7, 0xAE07},
	{0x2F90, 0xAF20, 0x2FB9, 0xAF09, 0x2FC2, 0xAF72, 0x2FEB, 0xAF5B,
	 0x2F34, 0xAF84, 0x2F1D, 0xAFAD, 0x2F66, 0xAFD6, 0x2F4F, 0xAFFF},
	{0x3078, 0xB0C8, 0x3051, 0xB0E1, 0x302A, 0xB09A, 0x3003, 0xB0B3,
	 0x30DC, 0xB06C, 0x30F5, 0xB045, 0x308E, 0xB03E, 0x30A7, 0xB017},
	{0x3180, 0xB130, 0x31A9, 0xB119, 0x31D2, 0xB162, 0x31FB, 0xB14B,
	 0x3124, 0xB194, 0x310D, 0xB1BD, 0x3176, 0xB1C6, 0x315F, 0xB1EF},
	{0x32C0, 0xB270, 0x32E9, 0xB259, 0x3292, 0xB222, 0x32BB, 0xB20B,
	 0x3264, 0xB2D4, 0x324D, 0xB2FD, 0x3236, 0xB286, 0x321F, 0xB2AF},
	{0x3338, 0xB388, 0x3311, 0xB3A1, 0x336A, 0xB3DA, 0x3343, 0xB3F3,
	 0x339C, 0xB32C, 0x33B5, 0xB305, 0x33CE, 0xB37E, 0x33E7, 0xB357},
	{0x3440, 0xB4F0, 0x3469, 0xB4D9, 0x3412, 0xB4A2, 0x343B, 0xB48B,
	 0x34E4, 0xB454, 0x34CD, 0xB47D, 0x34B6, 0xB406, 0x349F, 0xB42F},
	{0x35B8, 0xB508, 0x3591, 0xB521, 0x35EA, 0xB55A, 0x35C3, 0xB573,
	 0x351C, 0xB5AC, 0x3535, 0xB585, 0x354E, 0xB5FE, 0x3567, 0xB5D7},
	{0x36F8, 0xB648, 0x36D1, 0xB661, 0x36AA, 0xB61A, 0x3683, 0xB633,
	 0x365C, 0xB6EC, 0x3675, 0xB6C5, 0x360E, 0xB6BE, 0x3627, 0xB697},
	{0x3700, 0xB7B0, 0x3729, 0xB799, 0x3752, 0xB7E2, 0x377B, 0xB7CB,
	 0x37A4, 0xB714, 0x378D, 0xB73D, 0x37F6, 0xB746, 0x37DF, 0xB76F},
	{0x3808, 0xB8B8, 0x3821, 0xB891, 0x385A, 0xB8EA, 0x3873, 0xB8C3,
	 0x38AC, 0xB81C, 0x3885, 0xB835, 0x38FE, 0xB84E, 0x38D7, 0xB867},
	{0x39F0, 0xB940, 0x39D9, 0xB969, 0x39A2, 0xB912, 0x398B, 0xB93B,
	 0x3954, 0xB9E4, 0x397D, 0xB9CD, 0x3906, 0xB9B6, 0x392F, 0xB99F},
	{0x3AB0, 0xBA00, 0x3A99, 0xBA29, 0x3AE2, 0xBA52, 0x3ACB, 0xBA7B,
	 0x3A14, 0xBAA4, 0x3A3D, 0xBA8D, 0x3A46, 0xBAF6, 0x3A6F, 0xBADF},
	{0x3B48, 0xBBF8, 0x3B61, 0xBBD1, 0x3B1A, 0xBBAA, 0x3B33, 0xBB83,
	 0x3BEC, 0xBB5C, 0x3BC5, 0xBB75, 0x3BBE, 0xBB0E, 0x3B97, 0xBB27},
	{0x3C30, 0xBC80, 0x3C19, 0xBCA9, 0x3C62, 0xBCD2, 0x3C4B, 0xBCFB,
	 0x3C94, 0xBC24, 0x3CBD, 0xBC0D, 0x3CC6, 0xBC76, 0x3CEF, 0xBC5F},
	{0x3DC8, 0xBD78, 0x3DE1, 0xBD51, 0x3D9A, 0xBD2A, 0x3DB3, 0xBD03,
	 0x3D6C, 0xBDDC, 0x3D45, 0xBDF5, 0x3D3E, 0xBD8E, 0x3D17, 0xBDA7},
	{0x3E88, 0xBE38, 0x3EA1, 0xBE11, 0x3EDA, 0xBE6A, 0x3EF3, 0xBE43,
	 0x3E2C, 0xBE9C, 0x3E05, 0xBEB5, 0x3E7E, 0xBECE, 0x3E57, 0xBEE7},
	{0x3F70, 0xBFC0, 0x3F59, 0xBFE9, 0x3F22, 0xBF92, 0x3F0B, 0xBFBB,
	 0x3FD4, 0xBF64, 0x3FFD, 0xBF4D, 0x3F86, 0xBF36, 0x3FAF, 0xBF1F},
	{0x4048, 0xC0F8, 0x4061, 0xC0D1, 0x401A, 0xC0AA, 0x4033, 0xC083,
	 0x40EC, 0xC05C, 0x40C5, 0xC075, 0x40BE, 0xC00E, 0x4097, 0xC027},
	{0x41B0, 0xC100, 0x4199, 0xC129, 0x41E2, 0xC152, 0x41CB, 0xC17B,
	 0x4114, 0xC1A4, 0x413D, 0xC18D, 0x4146, 0xC1F6, 0x416F, 0xC1DF},
	{0x42F0, 0xC240, 0x42D9, 0xC269, 0x42A2, 0xC212, 0x428B, 0xC23B,
	 0x4254, 0xC2E4, 0x427D, 0xC2CD, 0x4206, 0xC2B6, 0x422F, 0xC29F},
	{0x4308, 0xC3B8, 0x4321, 0xC391, 0x435A, 0xC3EA, 0x4373, 0xC3C3,
	 0x43AC, 0xC31C, 0x4385, 0xC335, 0x43FE, 0xC34E, 0x43D7, 0xC367},
	{0x4470, 0xC4C0, 0x4459, 0xC4E9, 0x4422, 0xC492, 0x440B, 0xC4BB,
	 0x44D4, 0xC464, 0x44FD, 0xC44D, 0x4486, 0xC436, 0x44AF, 0xC41F},
	{0x4588, 0xC538, 0x45A1, 0xC511, 0x45DA, 0xC56A, 0x45F3, 0xC543,
	 0x452C, 0xC59C, 0x4505, 0xC5B5, 0x457E, 0xC5CE, 0x4557, 0xC5E7},
	{0x46C8, 0xC678, 0x46E1, 0xC651, 0x469A, 0xC62A, 0x46B3, 0xC603,
	 0x466C, 0xC6DC, 0x4645, 0xC6F5, 0x463E, 0xC68E, 0x4617, 0xC6A7},
	{0x4730, 0xC780, 0x4719, 0xC7A9, 0x4762, 0xC7D2, 0x474B, 0xC7FB,
	 0x4794, 0xC724, 0x47BD, 0xC70D, 0x47C6, 0xC776, 0x47EF, 0xC75F},
	{0x4838, 0xC888, 0x4811, 0xC8A1, 0x486A, 0xC8DA, 0x4843, 0xC8F3,
	 0x489C, 0xC82C, 0x48B5, 0xC805, 0x48CE, 0xC87E, 0x48E7, 0xC857},
	{0x49C0, 0xC970, 0x49E9, 0xC959, 0x4992, 0xC922, 0x49BB, 0xC90B,
	 0x4964, 0xC9D4, 0x494D, 0xC9FD, 0x4936, 0xC986, 0x491F, 0xC9AF},
	{0x4A80, 0xCA30, 0x4AA9, 0xCA19, 0x4AD2, 0xCA62, 0x4AFB, 0xCA4B,
	 0x4A24, 0xCA94, 0x4A0D, 0xCABD, 0x4A76, 0xCAC6, 0x4A5F, 0xCAEF},
	{0x4B78, 0xCBC8, 0x4B51, 0xCBE1, 0x4B2A, 0xCB9A, 0x4B03, 0xCBB3,
	 0x4BDC, 0xCB6C, 0x4BF5, 0xCB45, 0x4B8E, 0xCB3E, 0x4BA7, 0xCB17},
	{0x4C00, 0xCCB0, 0x4C29, 0xCC99, 0x4C52, 0xCCE2, 0x4C7B, 0xCCCB,
	 0x4CA4, 0xCC14, 0x4C8D, 0xCC3D, 0x4CF6, 0xCC46, 0x4CDF, 0xCC6F},
	{0x4DF8, 0xCD48, 0x4DD1, 0xCD61, 0x4DAA, 0xCD1A, 0x4D83, 0xCD33,
	 0x4D5C, 0xCDEC, 0x4D75, 0xCDC5, 0x4D0E, 0xCDBE, 0x4D27, 0xCD97},
	{0x4EB8, 0xCE08, 0x4E91, 0xCE21, 0x4EEA, 0xCE5A, 0x4EC3, 0xCE73,
	 0x4E1C, 0xCEAC, 0x4E35, 0xCE85, 0x4E4E, 0xCEFE, 0x4E67, 0xCED7},
	{0x4F40, 0xCFF0, 0x4F69, 0xCFD9, 0x4F12, 0xCFA2, 0x4F3B, 0xCF8B,
	 0x4FE4, 0xCF54, 0x4FCD, 0xCF7D, 0x4FB6, 0xCF06, 0x4F9F, 0xCF2F},
	{0x50A8, 0xD018, 0x5081, 0xD031, 0x50FA, 0xD04A, 0x50D3, 0xD063,
	 0x500C, 0xD0BC, 0x5025, 0xD095, 0x505E, 0xD0EE, 0x5077, 0xD0C7},
	{0x5150, 0xD1E0, 0x5179, 0xD1C9, 0x5102, 0xD1B2, 0x512B, 0xD19B,
	 0x51F4, 0xD144, 0x51DD, 0xD16D, 0x51A6, 0xD116, 0x518F, 0xD13F},
	{0x5210, 0xD2A0, 0x5239, 0xD289, 0x5242, 0xD2F2, 0x526B, 0xD2DB,
	 0x52B4, 0xD204, 0x529D, 0xD22D, 0x52E6, 0xD256, 0x52CF, 0xD27F},
	{0x53E8, 0xD358, 0x53C1, 0xD371, 0x53BA, 0xD30A, 0x5393, 0xD323,
	 0x534C, 0xD3FC, 0x5365, 0xD3D5, 0x531E, 0xD3AE, 0x5337, 0xD387},
	{0x5490, 0xD420, 0x54B9, 0xD409, 0x54C2, 0xD472, 0x54EB, 0xD45B,
	 0x5434, 0xD484, 0x541D, 0xD4AD, 0x5466, 0xD4D6, 0x544F, 0xD4FF},
	{0x5568, 0xD5D8, 0x5541, 0xD5F1, 0x553A, 0xD58A, 0x5513, 0xD5A3,
	 0x55CC, 0xD57C, 0x55E5, 0xD555, 0x559E, 0xD52E, 0x55B7, 0xD507},
	{0x5628, 0xD698, 0x5601, 0xD6B1, 0x567A, 0xD6CA, 0x5653, 0xD6E3,
	 0x568C, 0xD63C, 0x56A5, 0xD615, 0x56DE, 0xD66E, 0x56F7, 0xD647},
	{0x57D0, 0xD760, 0x57F9, 0xD749, 0x5782, 0xD732, 0x57AB, 0xD71B,
	 0x5774, 0xD7C4, 0x575D, 0xD7ED, 0x5726, 0xD796, 0x570F, 0xD7BF},
	{0x58D8, 0xD868, 0x58F1, 0xD841, 0x588A, 0xD83A, 0x58A3, 0xD813,
	 0x587C, 0xD8CC, 0x5855, 0xD8E5, 0x582E, 0xD89E, 0x5807, 0xD8B7},
	{0x5920, 0xD990, 0x5909, 0xD9B9, 0x5972, 0xD9C2, 0x595B, 0xD9EB,
	 0x5984, 0xD934, 0x59AD, 0xD91D, 0x59D6, 0xD966, 0x59FF, 0xD94F},
	{0x5A60, 0xDAD0, 0x5A49, 0xDAF9, 0x5A32, 0xDA82, 0x5A1B, 0xDAAB,
	 0x5AC4, 0xDA74, 0x5AED, 0xDA5D, 0x5A96, 0xDA26, 0x5ABF, 0xDA0F},
	{0x5B98, 0xDB28, 0x5BB1, 0xDB01, 0x5BCA, 0xDB7A, 0x5BE3, 0xDB53,
	 0x5B3C, 0xDB8C, 0x5B15, 0xDBA5, 0x5B6E, 0xDBDE, 0x5B47, 0xDBF7},
	{0x5CE0, 0xDC50, 0x5CC9, 0xDC79, 0x5CB2, 0xDC02, 0x5C9B, 0xDC2B,
	 0x5C44, 0xDCF4, 0x5C6D, 0xDCDD, 0x5C16, 0xDCA6, 0x5C3F, 0xDC8F},
	{0x5D18, 0xDDA8, 0x5D31, 0xDD81, 0x5D4A, 0xDDFA, 0x5D63, 0xDDD3,
	 0x5DBC, 0xDD0C, 0x5D95, 0xDD25, 0x5DEE, 0xDD5E, 0x5DC7, 0xDD77},
	{0x5E58, 0xDEE8, 0x5E71, 0xDEC1, 0x5E0A, 0xDEBA, 0x5E23, 0xDE93,
	 0x5EFC, 0xDE4C, 0x5ED5, 0xDE65, 0x5EAE, 0xDE1E, 0x5E87, 0xDE37},
	{0x5FA0, 0xDF10, 0x5F89, 0xDF39, 0x5FF2, 0xDF42, 0x5FDB, 0xDF6B,
	 0x5F04, 0xDFB4, 0x5F2D, 0xDF9D, 0x5F56, 0xDFE6, 0x5F7F, 0xDFCF},
	{0x60C0, 0xE070, 0x60E9, 0xE059, 0x6092, 0xE022, 0x60BB, 0xE00B,
	 0x6064, 0xE0D4, 0x604D, 0xE0FD, 0x6036, 0xE086, 0x601F, 0xE0AF},
	{0x6138, 0xE188, 0x6111, 0xE1A1, 0x616A, 0xE1DA, 0x6143, 0xE1F3,
	 0x619C, 0xE12C, 0x61B5, 0xE105, 0x61CE, 0xE17E, 0x61E7, 0xE157},
	{0x6278, 0xE2C8, 0x6251, 0xE2E1, 0x622A, 0xE29A, 0x6203, 0xE2B3,
	 0x62DC, 0xE26C, 0x62F5, 0xE245, 0x628E, 0xE23E, 0x62A7, 0xE217},
	{0x6380, 0xE330, 0x63A9, 0xE319, 0x63D2, 0xE362, 0x63FB, 0xE34B,
	 0x6324, 0xE394, 0x630D, 0xE3BD, 0x6376, 0xE3C6, 0x635F, 0xE3EF},
	{0x64F8, 0xE448, 0x64D1, 0xE461, 0x64AA, 0xE41A, 0x6483, 0xE433,
	 0x645C, 0xE4EC, 0x6475, 0xE4C5, 0x640E, 0xE4BE, 0x6427, 0xE497},
	{0x6500, 0xE5B0, 0x6529, 0xE599, 0x6552, 0xE5E2, 0x657B, 0xE5CB,
	 0x65A4, 0xE514, 0x658D, 0xE53D, 0x65F6, 0xE546, 0x65DF, 0xE56F},
	{0x6640, 0xE6F0, 0x6669, 0xE6D9, 0x6612, 0xE6A2, 0x663B, 0xE68B,
	 0x66E4, 0xE654, 0x66CD, 0xE67D, 0x66B6, 0xE606, 0x669F, 0xE62F},
	{0x67B8, 0xE708, 0x6791, 0xE721, 0x67EA, 0xE75A, 0x67C3, 0xE773,
	 0x671C, 0xE7AC, 0x6735, 0xE785, 0x674E, 0xE7FE, 0x6767, 0xE7D7},
	{0x68B0, 0xE800, 0x6899, 0xE829, 0x68E2, 0xE852, 0x68CB, 0xE87B,
	 0x6814, 0xE8A4, 0x683D, 0xE88D, 0x6846, 0xE8F6, 0x686F, 0xE8DF},
	{0x6948, 0xE9F8, 0x6961, 0xE9D1, 0x691A, 0xE9AA, 0x6933, 0xE983,
	 0x69EC, 0xE95C, 0x69C5, 0xE975, 0x69BE, 0xE90E, 0x6997, 0xE927},
	{0x6A08, 0xEAB8, 0x6A21, 0xEA91, 0x6A5A, 0xEAEA, 0x6A73, 0xEAC3,
	 0x6AAC, 0xEA1C, 0x6A85, 0xEA35, 0x6AFE, 0xEA4E, 0x6AD7, 0xEA67},
	{0x6BF0, 0xEB40, 0x6BD9, 0xEB69, 0x6BA2, 0xEB12, 0x6B8B, 0xEB3B,
	 0x6B54, 0xEBE4, 0x6B7D, 0xEBCD, 0x6B06, 0xEBB6, 0x6B2F, 0xEB9F},
	{0x6C88, 0xEC38, 0x6CA1, 0xEC11, 0x6CDA, 0xEC6A, 0x6CF3, 0xEC43,
	 0x6C2C, 0xEC9C, 0x6C05, 0xECB5, 0x6C7E, 0xECCE, 0x6C57, 0xECE7},
	{0x6D70, 0xEDC0, 0x6D59, 0xEDE9, 0x6D22, 0xED92, 0x6D0B, 0xEDBB,
	 0x6DD4, 0xED64, 0x6DFD, 0xED4D, 0x6D86, 0xED36, 0x6DAF, 0xED1F},
	{0x6E30, 0xEE80, 0x6E19, 0xEEA9, 0x6E62, 0xEED2, 0x6E4B, 0xEEFB,
	 0x6E94, 0xEE24, 0x6EBD, 0xEE0D, 0x6EC6, 0xEE76, 0x6EEF, 0xEE5F},
	{0x6FC8, 0xEF78, 0x6FE1, 0xEF51, 0x6F9A, 0xEF2A, 0x6FB3, 0xEF03,
	 0x6F6C, 0xEFDC, 0x6F45, 0xEFF5, 0x6F3E, 0xEF8E, 0x6F17, 0xEFA7},
	{0x7020, 0xF090, 0x7009, 0xF0B9, 0x7072, 0xF0C2, 0x705B, 0xF0EB,
	 0x7084, 0xF034, 0x70AD, 0xF01D, 0x70D6, 0xF066, 0x70FF, 0xF04F},
	{0x71D8, 0xF168, 0x71F1, 0xF141, 0x718A, 0xF13A, 0x71A3, 0xF113,
	 0x717C, 0xF1CC, 0x7155, 0xF1E5, 0x712E, 0xF19E, 0x7107, 0xF1B7},
	{0x7298, 0xF228, 0x72B1, 0xF201, 0x72CA, 0xF27A, 0x72E3, 0xF253,
	 0x723C, 0xF28C, 0x7215, 0xF2A5, 0x726E, 0xF2DE, 0x7247, 0xF2F7},
	{0x7360, 0xF3D0, 0x7349, 0xF3F9, 0x7332, 0xF382, 0x731B, 0xF3AB,
	 0x73C4, 0xF374, 0x73ED, 0xF35D, 0x7396, 0xF326, 0x73BF, 0xF30F},
	{0x7418, 0xF4A8, 0x7431, 0xF481, 0x744A, 0xF4FA, 0x7463, 0xF4D3,
	 0x74BC, 0xF40C, 0x7495, 0xF425, 0x74EE, 0xF45E, 0x74C7, 0xF477},
	{0x75E0, 0xF550, 0x75C9, 0xF579, 0x75B2, 0xF502, 0x759B, 0xF52B,
	 0x7544, 0xF5F4, 0x756D, 0xF5DD, 0x7516, 0xF5A6, 0x753F, 0xF58F},
	{0x76A0, 0xF610, 0x7689, 0xF639, 0x76F2, 0xF642, 0x76DB, 0xF66B,
	 0x7604, 0xF6B4, 0x762D, 0xF69D, 0x7656, 0xF6E6, 0x767F, 0xF6CF},
	{0x7758, 0xF7E8, 0x7771, 0xF7C1, 0x770A, 0xF7BA, 0x7723, 0xF793,
	 0x77FC, 0xF74C, 0x77D5, 0xF765, 0x77AE, 0xF71E, 0x7787, 0xF737},
	{0x7850, 0xF8E0, 0x7879, 0xF8C9, 0x7802, 0xF8B2, 0x782B, 0xF89B,
	 0x78F4, 0xF844, 0x78DD, 0xF86D, 0x78A6, 0xF816, 0x788F, 0xF83F},
	{0x79A8, 0xF918, 0x7981, 0xF931, 0x79FA, 0xF94A, 0x79D3, 0xF963,
	 0x790C, 0xF9BC, 0x7925, 0xF995, 0x795E, 0xF9EE, 0x7977, 0xF9C7},
	{0x7AE8, 0xFA58, 0x7AC1, 0xFA71, 0x7ABA, 0xFA0A, 0x7A93, 0xFA23,
	 0x7A4C, 0xFAFC, 0x7A65, 0xFAD5, 0x7A1E, 0xFAAE, 0x7A37, 0xFA87},
	{0x7B10, 0xFBA0, 0x7B39, 0xFB89, 0x7B42, 0xFBF2, 0x7B6B, 0xFBDB,
	 0x7BB4, 0xFB04, 0x7B9D, 0xFB2D, 0x7BE6, 0xFB56, 0x7BCF, 0xFB7F},
	{0x7C68, 0xFCD8, 0x7C41, 0xFCF1, 0x7C3A, 0xFC8A, 0x7C13, 0xFCA3,
	 0x7CCC, 0xFC7C, 0x7CE5, 0xFC55, 0x7C9E, 0xFC2E, 0x7CB7, 0xFC07},
	{0x7D90, 0xFD20, 0x7DB9, 0xFD09, 0x7DC2, 0xFD72, 0x7DEB, 0xFD5B,
	 0x7D34, 0xFD84, 0x7D1D, 0xFDAD, 0x7D66, 0xFDD6, 0x7D4F, 0xFDFF},
	{0x7ED0, 0xFE60, 0x7EF9, 0xFE49, 0x7E82, 0xFE32, 0x7EAB, 0xFE1B,
	 0x7E74, 0xFEC4, 0x7E5D, 0xFEED, 0x7E26, 0xFE96, 0x7E0F, 0xFEBF},
	{0x7F28, 0xFF98, 0x7F01, 0xFFB1, 0x7F7A, 0xFFCA, 0x7F53, 0xFFE3,
	 0x7F8C, 0xFF3C, 0x7FA5, 0xFF15, 0x7FDE, 0xFF6E, 0x7FF7, 0xFF47}
};

static struct host_transaction_context context;

/* enque inplementation */
static void en_queue(struct list_head *node, struct list_head *list)
{
	list_add_tail(node, list);
}

static struct list_head *de_queue(struct list_head *list)
{
	struct list_head *node = NULL;;

	if (!list_empty(list)) {
		node = list->next;
		list_del(node);
	}

	return node;
}

static struct list_head *peek(struct list_head *list)
{
	struct list_head *node = NULL;

	if (!list_empty(list))
		node = list->next;

	return node;
}

#define ADVANCE_BD(base,bd,bd_status)	\
	do{				\
		if(bd_status & R_W)	\
			bd = base;	\
		else			\
			bd++;		\
	}while(0)

static void push_dummy_bd(struct endpoint *ep)
{
	if (ep->already_pushed_dummy_bd == false) {
		u32 bd_status = *(u32 *) (ep->tx_empty_bd);
		ep->tx_empty_bd->buf = /*(u8*) */ DUMMY_BD_BUFFER;
		/*gets the next BD in the ring */
		ADVANCE_BD(ep->tx_base, ep->tx_empty_bd, bd_status);
		ep->already_pushed_dummy_bd = true;
	}
}

static void host_recycle_rx_bd(struct endpoint *ep)
{
	u32 status;

	status = *(u32 *) ep->rx_next_bd;
	status = R_I | R_E | (status & R_W);
	*(u32 *) (ep->rx_next_bd) = status;
}

static void dequeue_recycle_transaction(struct endpoint *ep0)
{
	struct list_head *node_list;
	node_list = de_queue(&(ep0->active_transactions_list));
	if (node_list)
		en_queue(node_list, &(ep0->empty_transactions_list));
	else
		printk("no transaction letf in the list\n");
}

void host_transmit_actual_frame(struct fhci_usb *usb)
{
	u16 tb_ptr;
	u32 bd_status;
	struct usb_bd *bd, *tmp_bd;
	struct endpoint *ep = usb->ep0;

	tb_ptr = ep->ep_pram_ptr->tx_bd_ptr;
	bd = (struct usb_bd *)qe_muram_addr(tb_ptr);

	if (bd->buf == DUMMY_BD_BUFFER) {
		ep->already_pushed_dummy_bd = false;

		tmp_bd = bd;
		bd_status = *(u32 *) bd;
		/*get the next BD in the ring */
		ADVANCE_BD(ep->tx_base, bd, bd_status);
		tb_ptr = (u16) qe_muram_offset((u32) bd);
		ep->ep_pram_ptr->tx_bd_ptr = tb_ptr;
		/*start transmit only if we have something in the TDs */
		if ((*(u32 *) bd) & T_R)
			usb->fhci->regs->usb_comm = USB_CMD_STR_FIFO;

		if (ep->tx_conf_bd->buf == DUMMY_BD_BUFFER) {
			tmp_bd->buf = 0;
			/*advance the BD pointer */
			ADVANCE_BD(ep->tx_base, ep->tx_conf_bd, bd_status);
		} else
			tmp_bd->buf = /*(u8*) */ DUMMY2_BD_BUFFER;
	}
}

/* Flush all transmitted packets from TDs in the actual frame.
 * This routine is called when something wrong with the controller and 
 * we want to get rid of the actual frame and start again next frame
 */
void flush_actual_frame(struct fhci_usb *usb)
{
	flush_all_transmissions(usb);
}

/* get next transaction */
static void inc_transaction(struct endpoint *ep0)
{
	/*if this is the last element in the list */

	if (ep0->conf_transaction->node.next ==
	    &(ep0->active_transactions_list))
		ep0->conf_transaction = NULL;
	else
		ep0->conf_transaction =
		    list_entry(ep0->conf_transaction->node.next,
			       struct transaction, node);
}

static void enqueue_to_active_transactions(struct endpoint *ep0,
					   struct transaction *trans)
{
	en_queue(&(trans->node), &(ep0->active_transactions_list));
	if (ep0->conf_transaction == NULL)
		ep0->conf_transaction = trans;
}

/*transaction queue operations*/
static struct transaction *get_active_transaction(struct endpoint *ep0)
{
	struct transaction *trans = NULL;
	struct list_head *list = de_queue(&(ep0->active_transactions_list));

	if (list)
		trans = list_entry(list, struct transaction, node);
	return trans;
}

static struct transaction *get_empty_transaction(struct endpoint *ep0)
{
	struct transaction *trans = NULL;
	struct list_head *list = de_queue(&(ep0->empty_transactions_list));

	if (list)
		trans = list_entry(list, struct transaction, node);
	return trans;
}

static struct transaction *peek_transaction(struct endpoint *ep0)
{
	struct transaction *trans = NULL;
	struct list_head *list = peek(&(ep0->active_transactions_list));

	if (list)
		trans = list_entry(list, struct transaction, node);
	return trans;
}

/*tokens queue operations*/
static struct token *get_empty_token(struct endpoint *ep0)
{
	struct token *tk = NULL;
	struct list_head *list = de_queue(&(ep0->empty_tokens_list));

	if (list)
		tk = list_entry(list, struct token, node);

	return tk;
}

static void set_ep0_parameters(struct fhci_usb *usb,
			       enum fhci_tf_mode trans_mode)
{
	struct endpoint *ep0 = NULL;
	u16 tmp_ep = 0;

	ep0 = usb->ep0;

	tmp_ep = usb->fhci->regs->usb_ep[0];
	if (trans_mode == FHCI_TF_ISO)
		tmp_ep |= USB_TRANS_ISO;
	else
		tmp_ep &= ~USB_TRANS_ISO;
	usb->fhci->regs->usb_ep[0] = tmp_ep;
}

static u32 endpoint_link_tx_bd_to_token(struct endpoint *ep, struct usb_bd **bd)
{
	u32 bd_status;

	/*start from the next BD that should be filled */
	*bd = ep->tx_empty_bd;

	bd_status = *(u32 *) (*bd);

	if (!(bd_status & (T_R | USB_BD_LENGTH_MASK)))
		return 0;
	else
		return -1;
}

static u32 endpoint_link_rx_bd_to_user_buffer(struct endpoint *ep, u32 buffer,
					      struct usb_bd **bd)
{
	u32 bd_status;

	/*starts frome the next BD that should be filled */
	*bd = ep->rx_empty_bd;
	bd_status = *(u32 *) (*bd);

	if (!(bd_status & R_E))
		return -2;

	(*bd)->buf = buffer;
	/*get the next BD in the ring */
	if (bd_status & R_W)
		ep->rx_empty_bd = ep->rx_base;
	else
		ep->rx_empty_bd++;

	return 0;
}

static u32 endpoint_link_tx_bd_to_token_data(struct endpoint *ep,
					     struct usb_bd **token_bd,
					     struct usb_bd **data_bd)
{
	u32 bd_status;

	/*starts from the next BD that should be filled */
	*token_bd = ep->tx_empty_bd;
	bd_status = *(u32 *) (*token_bd);
	if (bd_status & (T_R | USB_BD_LENGTH_MASK))
		return -1;

	/*get the next BD in the ring */
	if (bd_status & R_W)
		*data_bd = ep->tx_base;
	else
		*data_bd =
		    (struct usb_bd *)(((u32) (*token_bd)) + SIZEOF_USB_BD);
	bd_status = *(u32 *) (*data_bd);
	if (bd_status & (T_R | USB_BD_LENGTH_MASK))
		return -1;

	return 0;

}

/* The application calls this function for transmitting a data frame on a
 * specified endpoint of the USB device.The frame is put in the driver's
 * transmit queue for this endpoint,then,an internal routine,kick_tx is called
 * to handle this frame
 */
static u32 transmit_packet(struct fhci_usb *usb, struct packet *pkt)
{
	struct endpoint *ep = NULL;
	struct usb_bd *bd;
	u32 bd_status, pid_mask;

	ep = usb->ep0;

	/*starts from the next BD that should be filled */
	bd = ep->tx_empty_bd;
	bd_status = *(u32 *) bd;
	if (!(bd_status & (T_R | USB_BD_LENGTH_MASK))) {	/*if the BD is free */
		pkt->priv_data = (void *)bd;
		/*sets up the buffer descriptor */
		bd->buf = virt_to_phys((void *)pkt->data);
		bd_status = (bd_status & T_W);
		if (!(pkt->info & PKT_NO_CRC))
			bd_status |= (T_R | T_I | T_L | T_TC | pkt->len);
		else
			bd_status |= (T_R | T_I | T_L | pkt->len);

		if (ep->data_0_1) {
			pid_mask = T_PID_DATA1;
			pkt->info |= PKT_PID_DATA1;
		} else {
			pid_mask = T_PID_DATA0;
			pkt->info |= PKT_PID_DATA0;
		}

		/*if the frame is a data frame (not a token),
		 *set pid field according to the data01 toggle 
		 *and toggle the data01 field of the endpoint*/
		if (pkt->info & (PKT_HOST_DATA | PKT_HOST_COMMAND))
			bd_status |= pid_mask;

		/*set T_CNF if indicated by the application */
		if (pkt->info & PKT_SET_HOST_LAST)
			bd_status |= T_CNF;

		/*set T_LSP if indicated by the application */
		if (pkt->info & PKT_LOW_SPEED_PACKET)
			bd_status |= T_LSP;
		*(u32 *) bd = bd_status;

		/*get the next BD in the ring */
		if (bd_status & T_W)
			bd = ep->tx_base;
		else
			bd++;

		/*puts the frame to the confirm queue */
		cq_put(ep->conf_frame_Q, pkt);
		/*sae parameters for next call */
		ep->tx_empty_bd = bd;

		if (pkt->info & PKT_SET_HOST_LAST)
			usb->fhci->regs->usb_comm = USB_CMD_STR_FIFO;
		return 0;
	} else
		return -1;
}

/* Submitting a data frame to a specified endpoint of a USB device
 * The frame is put in the driver's transmit queue for this endpoint
 *
 * Arguments:
 * usb		A pointer to the USB structure
 * pkt		A pointer to the user frame structure
 * trans_type	Transaction tyep - IN,OUT or SETUP
 * dest_addr	Device address - 0~127
 * dest_ep	Endpoint number of the device - 0~16
 * trans_mode	Pipe type - ISO,Interrupt,bulk or control
 * dest_speed	USB speed - Low speed or FULL speed
 * data_toggle	Data sequence toggle - 0 or 1
 */
u32 host_transaction(struct fhci_usb * usb,
		     struct packet * pkt,
		     e_usb_trans_type trans_type,
		     u8 dest_addr,
		     u8 dest_ep,
		     enum fhci_tf_mode trans_mode,
		     enum fhci_speed dest_speed, u8 data_toggle)
{
	/*This function will be fixed in the future */
	struct endpoint *ep0 = NULL;
	struct packet *user_frame = pkt;
	struct packet *token_frame = NULL;
	struct transaction *trans = NULL;
	struct token *tk = NULL;
	e_host_transaction_status last_status;

	/*check and catch busy flag */
	fhci_usb_disable_interrupt(usb);
	last_status = context.curr_trans_status;
	context.curr_trans_status = e_HOST_TRANS_BUSY;
	fhci_usb_enable_interrupt(usb);

	if (last_status == e_HOST_TRANS_BUSY)
		return -EBUSY;

	/*since we are here curr_trans_status was either FREE or WAIT. It was 
	 *set to BUSY.if it was in WAIT status and is being continued 
	 *(NULL parameters). We simply retrieve the old context, set the 
	 *endpoint to the new status and continue as if it was a regulare host 
	 *transaction request in FREE status.regulare attempts to start 
	 *a transaction while waiting will fail with BUSY.
	 */
	if (last_status == e_HOST_TRANS_WAIT) {
		if (usb != NULL || pkt != NULL)
			return -EBUSY;

		usb = (struct fhci_usb *)context.usb;
		user_frame = context.user_frame;
		trans_type = context.transaction_type;
		dest_addr = context.dest_addr;
		dest_ep = context.dest_ep;
		trans_mode = context.dest_trans_mode;
		dest_speed = context.dest_speed;
		data_toggle = context.data_toggle;
		set_ep0_parameters(usb, trans_mode);
	}

	ep0 = usb->ep0;

	/*if the current transfer mode, the one being served by the CPM , is 
	 *different from the new transfer mode, the one we are being asked to
	 *send and one of them  is ISO transfr. We need to  wait till all the
	 *BDs from the previous mode are over, change the endpoint parameters
	 *and start sending the new transaction.
	 *since this is  a costly transformation from  one mode to the other,
	 *there is a  threshold which  counts how  many transaction  asked to
	 *switch from one mode to the other. Untill the treshold is reached a
	 *BUSY is returned to indicate that this transaction was not accepted
	 *and should be  retried (later). Once the treshold  has been reached
	 *the function will wait for the BDs to clear and will not accept any
	 *new transaction till then.
	 */
	if (context.curr_trans_mode != trans_mode &&
	    (context.curr_trans_mode == FHCI_TF_ISO || trans_mode == FHCI_TF_ISO)) {
		if (context.threshold) {
			/*threshold has not beed reached,
			 *reject the current transaction request*/
			context.threshold--;
			context.curr_trans_status = e_HOST_TRANS_FREE;
			return -EBUSY;
		} else {
			/*threshold has been reached.save the current context */
			context.usb = usb;
			context.user_frame = user_frame;
			context.transaction_type = trans_type;
			context.dest_addr = dest_addr;
			context.dest_ep = dest_ep;
			context.dest_trans_mode = trans_mode;
			context.dest_speed = dest_speed;
			context.data_toggle = data_toggle;
			context.curr_trans_status = e_HOST_TRANS_WAIT;
			context.curr_trans_mode = trans_mode;
			context.threshold =
			    (u8) ((trans_mode ==
				   FHCI_TF_CTRL) ? ISO_THRESHOLD :
				  OTHER_THRESHOLD);

			/*wait until all active transaction will be submitted */
			if (!list_empty(&(ep0->active_transactions_list))) {
				if (schedule_work(&(ep0->host_transaction_bh))) {
					printk("can't schedule bootm-half\n");
					return -EBUSY;
				}
				context.curr_trans_status = e_HOST_TRANS_WAIT;

				return 0;
			} else
				set_ep0_parameters(usb, trans_mode);
		}
	}

	ep0->data_0_1 = data_toggle;

	/*allocat objects for the transaction */
	token_frame = cq_get(ep0->empty_frame_Q);
	trans = get_empty_transaction(ep0);
	tk = get_empty_token(ep0);

	/*allocate a frame which contains a token for transmiting 
	 *from the host to the device endpoint*/
	switch (trans_type) {
	case FHCI_TA_OUT:
		token_frame->info = PKT_TOKEN_FRAME |
		    PKT_OUT_TOKEN_FRAME | PKT_NO_CRC;
		tk->token_packet.pid = OUT_TOKEN;
		break;
	case FHCI_TA_SETUP:
		token_frame->info = PKT_TOKEN_FRAME |
		    PKT_SETUP_TOKEN_FRAME | PKT_NO_CRC;
		tk->token_packet.pid = SETUP_TOKEN;
		break;
	case FHCI_OP_IN:
		token_frame->info = PKT_TOKEN_FRAME | PKT_IN_TOKEN_FRAME |
		    PKT_NO_CRC | PKT_SET_HOST_LAST;
		tk->token_packet.pid = IN_TOKEN;
		break;
	default:
		context.curr_trans_status = e_HOST_TRANS_FREE;
		return -EINVAL;
	}

	if ((dest_speed == FHCI_LOW_SPEED) &&
	    (usb->port_status == FHCI_PORT_FULL))
		token_frame->info |= PKT_LOW_SPEED_PACKET;

	tk->token_packet.pkt = token_pkts[dest_addr][dest_ep];
	token_frame->data = (u8 *) (&tk->token_packet);
	token_frame->len = 3;
	token_frame->status = USB_TD_OK;	/*packet's status */

	trans->token_frame = token_frame;
	trans->data_frame = user_frame;
	trans->token = tk;
	trans->transaction_status = e_TRANSACTION_WAIT_FOR_TOKEN_CONF;
	trans->mode = trans_mode;
	trans->speed = dest_speed;
	trans->pid = data_toggle;
	trans->type = trans_type;

	if (trans_type == FHCI_OP_IN) {
		if (endpoint_link_tx_bd_to_token(ep0, &(trans->token_bd)) != 0) {
			en_queue(&(tk->node), &(ep0->empty_tokens_list));
			recycle_frame(usb, token_frame);
			en_queue(&trans->node, &ep0->empty_transactions_list);
			context.curr_trans_status = e_HOST_TRANS_FREE;
			return -1;
		}
		if (endpoint_link_rx_bd_to_user_buffer(ep0,
						       virt_to_phys((void *)
								    user_frame->
								    data),
						       &(trans->data_bd)) != 0)
			printk("false\n");

		/*enqueue transaction object to the active transactions list */
		enqueue_to_active_transactions(ep0, trans);

		fhci_usb_disable_interrupt(usb);
		/*first transmit an "IN" token */
		if (transmit_packet(usb, token_frame) != 0)
			printk("false\n");
	} else {
		if (trans_type == FHCI_TA_OUT)
			user_frame->info |= PKT_SET_HOST_LAST | PKT_HOST_DATA;
		else
			user_frame->info |=
			    PKT_SET_HOST_LAST | PKT_HOST_COMMAND;

		if (endpoint_link_tx_bd_to_token_data(ep0, &(trans->token_bd),
						      &(trans->data_bd)) != 0) {
			en_queue(&(tk->node), &(ep0->empty_tokens_list));
			recycle_frame(usb, token_frame);
			en_queue(&trans->node, &ep0->empty_transactions_list);
			context.curr_trans_status = e_HOST_TRANS_FREE;
			return -1;
		}

		/*enqueue transaction object to the active transaction list */
		enqueue_to_active_transactions(ep0, trans);

		/*first transmit an "OUT"/"SETUP" token */
		if (transmit_packet(usb, token_frame) != 0)
			printk("false\n");

		if ((dest_speed == FHCI_LOW_SPEED) &&
		    (usb->port_status == FHCI_PORT_FULL))
			user_frame->info |= PKT_LOW_SPEED_PACKET;

		fhci_usb_disable_interrupt(usb);
		/*now transmit the data */
		if (transmit_packet(usb, user_frame) != 0)
			printk("false\n");
	}

	context.curr_trans_status = e_HOST_TRANS_FREE;

	fhci_usb_enable_interrupt(usb);
	return 0;
}

static void wait_bds_empty_bh(void *data)
{
	struct fhci_usb *usb = (struct fhci_usb *)data;

	while (!list_empty(&(usb->ep0->active_transactions_list))) ;
	host_transaction(NULL, NULL, (e_usb_trans_type) 0, 0, 0,
			 (enum fhci_tf_mode) 0, (enum fhci_speed) 0, 0);
}

/* destroy an USB endpoint */
void endpoint_zero_free(struct fhci_usb *usb)
{
	struct endpoint *ep;
	struct token *tk;
	struct transaction *trans;
	int size;

	ep = usb->ep0;

	if (ep) {
		if (ep->rx_base)
			qe_muram_free(qe_muram_offset((u32) ep->rx_base));

		if (ep->conf_frame_Q) {
			size = cq_howmany(ep->conf_frame_Q);
			for (; size; size--) {
				struct packet *pkt = cq_get(ep->conf_frame_Q);
				kfree(pkt);
			}
			cq_delete(ep->conf_frame_Q);
		}

		if (ep->empty_frame_Q) {
			size = cq_howmany(ep->empty_frame_Q);
			for (; size; size--) {
				struct packet *pkt = cq_get(ep->empty_frame_Q);
				kfree(pkt);
			}
			cq_delete(ep->empty_frame_Q);
		}

		if (ep->dummy_packets_Q) {
			size = cq_howmany(ep->dummy_packets_Q);
			for (; size; size--) {
				u8 *buff = cq_get(ep->dummy_packets_Q);
				kfree((void *)buff);
			}
			cq_delete(ep->dummy_packets_Q);
		}

		if (ep->pkt)
			kfree(ep->pkt);

		trans = get_empty_transaction(ep);
		while (trans) {
			kfree(trans);
			trans = get_empty_transaction(ep);
		}

		trans = get_active_transaction(ep);
		while (trans) {
			kfree(trans);
			trans = get_active_transaction(ep);
		}

		tk = get_empty_token(ep);
		while (tk) {
			kfree(tk);
			tk = get_empty_token(ep);
		}

		kfree(ep);
		usb->ep0 = NULL;
	}
}

/* create the endpoint structure */
u32 create_endpoint(struct fhci_usb *usb, enum fhci_mem_alloc data_mem,
		u32 ring_len)
{
	struct endpoint *ep;
	struct usb_bd *bd;
	int ep_mem_size;
	u32 ep_ptr;
	u32 i;
	u32 tmp_val;

	/*we need at least 3 BDs in the ring */
	if (!(ring_len) > 2) {
		printk("Illegal BD ring length parameters!\n");
		return -EINVAL;
	}

	/*allocate memory for the endpoint data structure */
	ep = kzalloc(sizeof(*ep), GFP_KERNEL);
	if (!ep) {
		printk("no memory for endpoint zero objcet\n");
		return -ENOMEM;
	}

	/*initialize endpoint structure */
	ep->bd_ring_len = ring_len;

	/*initialize BD memory,first check if there is enough memory 
	 *for all requested BD's and for the endpoint parameter ram
	 */
	ep_mem_size = (int)((ep->bd_ring_len * SIZEOF_USB_BD * 2) +
			    sizeof(struct fhci_ep_pram));
	ep->rx_base = (struct usb_bd *)qe_muram_addr(qe_muram_alloc(ep_mem_size, 32));
	if (ep->rx_base == (void *)(~0)) {
		endpoint_zero_free(usb);
		printk("no memory for endpoint PRAM\n");
		return -ENOMEM;
	}

	ep->tx_base = (struct usb_bd *)((u32) ep->rx_base +
					(SIZEOF_USB_BD * ep->bd_ring_len));

	/*zero all queue pointer */
	ep->conf_frame_Q = cq_new(ep->bd_ring_len + 3);
	ep->empty_frame_Q = cq_new(ep->bd_ring_len + 3);
	ep->dummy_packets_Q = cq_new(ep->bd_ring_len + 3);
	if (!ep->conf_frame_Q || !ep->empty_frame_Q || !ep->dummy_packets_Q) {
		endpoint_zero_free(usb);
		printk("no memory for frames queues\n");
		return -ENOMEM;
	}
	tmp_val = ep->bd_ring_len + 2;
	for (i = 0; i < tmp_val; i++) {
		struct packet *pkt =
		    (struct packet *)kmalloc(sizeof(struct packet), GFP_KERNEL);
		u8 *buff = (u8 *) kmalloc(1028 * sizeof(u8), GFP_KERNEL);
		if (!pkt) {
			endpoint_zero_free(usb);
			printk("no memory for frame object\n");
			return -ENOMEM;
		}
		if (!buff) {
			endpoint_zero_free(usb);
			printk("no memory for buffer object\n");
			return -ENOMEM;
		}
		cq_put(ep->empty_frame_Q, pkt);
		cq_put(ep->dummy_packets_Q, buff);
	}

	INIT_LIST_HEAD(&ep->empty_tokens_list);
	INIT_LIST_HEAD(&ep->empty_transactions_list);
	INIT_LIST_HEAD(&ep->active_transactions_list);
	tmp_val = ep->bd_ring_len + 2;
	for (i = 0; i < tmp_val; i++) {
		struct transaction *trans =
		    (struct transaction *)kmalloc(sizeof(struct transaction),
						  GFP_KERNEL);
		struct token *tk = (struct token *)kmalloc(sizeof(struct token),
							   GFP_KERNEL);
		if (!trans) {
			endpoint_zero_free(usb);
			printk("no memory for transaction object\n");
			return -ENOMEM;
		}
		if (!tk) {
			endpoint_zero_free(usb);
			printk("no memory for TX token object\n");
			return -ENOMEM;
		}
		list_add_tail(&(trans->node), &(ep->empty_transactions_list));
		list_add_tail(&(tk->node), &(ep->empty_tokens_list));
	}

	/*initialize the Bottom-Half routine */
	INIT_WORK(&ep->host_transaction_bh, wait_bds_empty_bh, usb);

	/*allocate memory for RX frame */
	ep->pkt = (struct packet *)kmalloc(sizeof(struct packet), GFP_KERNEL);
	if (!ep->pkt) {
		endpoint_zero_free(usb);
		printk("no memory for RX frame object\n");
		return -ENOMEM;
	}

	/*we put the endpoint parameter RAM right behind the TD ring
	 *the ep_ptr must be aligned
	 */
	ep_ptr = (u32) ((u32) ep->tx_base + (SIZEOF_USB_BD * ep->bd_ring_len));
	ep->ep_pram_ptr = (struct fhci_ep_pram *)ep_ptr;

	ep->conf_transaction = NULL;

	ep->tx_conf_bd = ep->tx_base;
	ep->tx_empty_bd = ep->tx_base;
	ep->rx_next_bd = ep->rx_base;
	ep->rx_empty_bd = ep->rx_base;
	ep->was_busy_before_nacked = false;
	ep->nack_nesting_counter = 0;

	ep->data_0_1 = 0;	/*data 0 */

	/*initialize Tx bds */
	bd = ep->tx_base;
	for (i = 0; i < ep->bd_ring_len; i++) {
		bd->buf = 0;
		*(u32 *) bd = 0;	/*bd's status and length init to 0 */
		bd++;
	}
	bd--;
	*(u32 *) bd = T_W;	/*for last BD set Wrap bit */

	/*initialize Rx bds */
	bd = ep->rx_base;
	for (i = 0; i < ep->bd_ring_len; i++) {
		bd->buf = 0;
		*(u32 *) bd = (R_E | R_I);
		bd++;
	}
	bd--;
	*(u32 *) bd = (R_I | R_E | R_W);	/*for last BD set Wrap bit */

	/*updage the host_transaction context object */
	context.usb = NULL;
	context.user_frame = NULL;
	context.transaction_type = (e_usb_trans_type) 0;
	context.dest_addr = 0;
	context.dest_ep = 0;
	context.dest_trans_mode = (enum fhci_tf_mode) 0;
	context.dest_speed = (enum fhci_speed) 0;
	context.data_toggle = 0;
	context.curr_trans_status = e_HOST_TRANS_FREE;
	context.curr_trans_mode = FHCI_TF_CTRL;
	context.threshold = 0;

	usb->ep0 = ep;

	return 0;
}

/* initialize the endpoint register according to the given parameters 
 * Arguments:
 * usb		A pointer to the data sturcture of the USB
 * ep		A pointer to the endpoint structure
 * data_mem	Teh data memory partition(BUS)
 */
void init_endpoint_registers(struct fhci_usb *usb,
			     struct endpoint *ep, enum fhci_mem_alloc data_mem)
{
	u8 rt;

	/*set the endpoint register according to the endpoint */
	usb->fhci->regs->usb_ep[0] = (USB_TRANS_CTR | USB_EP_MF);

	usb->fhci->pram->ep_ptr[0] = qe_muram_offset((u32) ep->ep_pram_ptr);

	rt = (BUS_MODE_BO_BE | BUS_MODE_GBL);
#ifdef MULTI_DATA_BUS
	if (data_mem == MEM_SECONDARY)
		rt |= BUS_MODE_DTB;
#endif
	ep->ep_pram_ptr->rx_func_code = rt;
	ep->ep_pram_ptr->tx_func_code = rt;

	ep->ep_pram_ptr->rx_buff_len = 1028;

	ep->ep_pram_ptr->rx_base = qe_muram_offset((u32) ep->rx_base);
	ep->ep_pram_ptr->tx_base = qe_muram_offset((u32) ep->tx_base);
	ep->ep_pram_ptr->rx_bd_ptr = qe_muram_offset((u32) ep->rx_base);
	ep->ep_pram_ptr->tx_bd_ptr = qe_muram_offset((u32) ep->tx_base);
	ep->ep_pram_ptr->tx_state = 0;
}

/* local to the bds */
static int data_confirmation_callback(struct fhci_usb *usb,
				      struct packet *pkt_data,
				      struct usb_bd *bd, u32 status)
{
	struct endpoint *ep0 = NULL;
	struct transaction *trans = NULL;
	struct packet *pkt_user = NULL;

	ep0 = usb->ep0;
	trans = ep0->conf_transaction;

	if ((bd != trans->data_bd) ||
	    trans->data_frame->data != pkt_data->data) {
		printk("lost synchronization with BD(data) ring\n");
		return TRANS_FAIL;
	}

	pkt_user = trans->data_frame;
	if (trans->type == FHCI_OP_IN) {
		/*unlink this buffer in the RX BD */
		trans->data_bd->buf = 0;

		if (!(pkt_data->status & USB_TD_ERROR)) {
			if ((trans->mode != FHCI_TF_ISO) &&
			    (((pkt_user->info & PKT_PID_DATA0) &&
			      !(pkt_data->info & PKT_PID_DATA0)) ||
			     ((pkt_user->info & PKT_PID_DATA1) &&
			      !(pkt_data->info & PKT_PID_DATA1))))
				pkt_user->status = USB_TD_RX_ER_PID;
			else if (pkt_user->len > pkt_data->len)
				pkt_user->status = USB_TD_RX_DATA_UNDERUN;
		} else
			pkt_user->status = pkt_data->status;
	} else
		pkt_user->status = pkt_data->status;
	pkt_user->len = pkt_data->len;
	if (pkt_user->info & PKT_DUMMY_PACKET)
		pkt_user->info = (pkt_data->info | PKT_DUMMY_PACKET);
	else
		pkt_user->info = pkt_data->info;

	inc_transaction(ep0);
	if (trans->transaction_status == e_TRANSACTION_WAIT_FOR_TOKEN_CONF)
		trans->transaction_status = e_TRANSACTION_DATA_ALLREADY_CONF;
	else
		dequeue_recycle_transaction(ep0);

	/*inform the user about the transmited frame completion */
	transaction_confirm(usb, pkt_user);

	return TRANS_OK;
}

/* local to the bds */
static void endpoint_unlink_advance_rx_pointers(struct endpoint *ep,
						struct usb_bd *bd)
{
	struct usb_bd *next_rx_bd;
	u16 rb_ptr;
	u32 bd_status;

	/*link this buffer to NULL buffer */
	bd->buf = 0;

	/*advance microcode pointer to the next BD */
	bd_status = *(u32 *) bd;
	rb_ptr = ep->ep_pram_ptr->rx_bd_ptr;
	next_rx_bd = ep->rx_next_bd;
	/*get the next BD in the ring */
	if (bd_status & R_W) {
		rb_ptr = ep->ep_pram_ptr->rx_base;
		next_rx_bd = ep->rx_base;
	} else {
		rb_ptr += SIZEOF_USB_BD;
		next_rx_bd++;
	}
	ep->ep_pram_ptr->rx_bd_ptr = rb_ptr;
	ep->rx_next_bd = next_rx_bd;

	inc_transaction(ep);
}

/* local to the bds */
static void endpoint_unlink_advance_tx_bd(struct endpoint *ep,
					  struct usb_bd *bd)
{
	struct packet *pkt;
	u16 tb_ptr = 0;
	u32 bd_status;

	/*Mark BD as empty */
	*(u32 *) bd = ((*(u32 *) bd) & T_W);
	bd->buf = 0;

	/*advance microcode pointer to the next BD */
	bd_status = *(u32 *) bd;
	tb_ptr = ep->ep_pram_ptr->tx_bd_ptr;
	/*gets the next BD in the ring */
	if (bd_status & T_W)
		tb_ptr = ep->ep_pram_ptr->tx_base;
	else
		tb_ptr += SIZEOF_USB_BD;
	ep->ep_pram_ptr->tx_bd_ptr = tb_ptr;

	/*advance frame comfirmation queue */
	pkt = cq_get(ep->conf_frame_Q);	/*MAYBE,need to fixed */
}

/* local to thw bds */
static int token_confirmation_callback(struct fhci_usb *usb,
				       struct packet *token_frame,
				       struct usb_bd *bd, u32 bd_status)
{
	struct endpoint *ep = NULL;
	struct transaction *trans = NULL;
	struct packet *user_frame = NULL;
	int ans = TRANS_OK;
	u32 frame_status;

	ep = usb->ep0;
	trans = peek_transaction(ep);
	if (!trans)
		return TRANS_DISCARD;

	if ((bd != trans->token_bd) || (trans->token_frame != token_frame)) {
		printk("lost synchronizeation with BD(token) ring\n");
		return TRANS_FAIL;
	}

	/*recycle the token object */
	if (trans->token) {
		en_queue(&(trans->token->node), &(ep->empty_tokens_list));
		trans->token = NULL;
	}
	frame_status = token_frame->status;
	recycle_frame(usb, token_frame);
	trans->token_frame = NULL;

	/*check the status of the token transmission */
	if (bd_status == 0) {	/*status is OK */
		if (trans->transaction_status ==
		    e_TRANSACTION_WAIT_FOR_TOKEN_CONF) {
			trans->transaction_status =
			    e_TRANSACTION_WAIT_FOR_DATA_CONF;
			ans = TRANS_INPROGRESS;
		} else
			dequeue_recycle_transaction(ep);
	} else {		/*token transmission failed */
		if (trans->type == FHCI_OP_IN)
			endpoint_unlink_advance_rx_pointers(ep, trans->data_bd);
		else if (bd_status & T_UN) {
			endpoint_unlink_advance_tx_bd(ep, trans->data_bd);
			ans = TRANS_DISCARD;
		} else
			printk("illegal ack from device\n");

		/*update the user frame with the transaction status */
		trans->data_frame->status = frame_status;
		trans->data_frame->len = 0;

		user_frame = trans->data_frame;
		dequeue_recycle_transaction(ep);

		/*inform the user about the transmitted frame completion */
		transaction_confirm(usb, user_frame);
	}

	return ans;
}

/* Handles the receive packet */
u32 receive_packet_interrupt(struct fhci_hcd * fhci)
{
	struct fhci_usb *usb = (struct fhci_usb *)fhci->usb_lld;
	struct endpoint *ep = NULL;
	struct packet *pkt = NULL;
	struct usb_bd *bd = NULL;
	u32 bd_status, length;

	ep = usb->ep0;
	pkt = ep->pkt;

	/*collect receive buffers */
	bd = ep->rx_next_bd;
	bd_status = *(u32 *) bd;
	length = bd_status & USB_BD_LENGTH_MASK;
	/*while there are received buffers:BD is not empty */
	if (!(bd_status & R_E) && length) {
		/*if there is no error on the frame */
		if ((bd_status & R_F) && (bd_status & R_L) &&
		    !(bd_status & R_ERROR)) {
			pkt->len = (length - CRC_SIZE);
			pkt->status = USB_TD_OK;
			switch (bd_status & R_PID) {
			case R_PID_DATA1:
				pkt->info = PKT_PID_DATA1;
				break;
			case R_PID_SETUP:
				pkt->info = PKT_PID_SETUP;
				break;
			default:
				pkt->info = PKT_PID_DATA0;
				break;
			}
		} else {	/*if there are some error on the frame */
			/*this assumes there is only one error on the BD
			 *it is not correct if there are more.*/
			pkt->len = 0;
			if (bd_status & R_NO)
				pkt->status = USB_TD_RX_ER_NONOCT;
			else if (bd_status & R_AB)
				pkt->status = USB_TD_RX_ER_BITSTUFF;
			else if (bd_status & R_CR)
				pkt->status = USB_TD_RX_ER_CRC;
			else if (bd_status & R_OV)
				pkt->status = USB_TD_RX_ER_OVERUN;
			else if (bd_status & R_F)
				pkt->status = USB_TD_RX_ER_BITSTUFF;
			else if (bd_status & R_L)
				pkt->status = USB_TD_RX_ER_BITSTUFF;
			else {
				printk("receive error occured\n");
				return -1;
			}
		}

		pkt->data = (u8 *) phys_to_virt(bd->buf);
		if (data_confirmation_callback(usb, pkt, bd,
					       (bd_status & R_ERROR)) ==
		    TRANS_FAIL)
			flush_all_transmissions(usb);
		*(u32 *) bd = (bd_status & USB_BD_STATUS_MASK);
		host_recycle_rx_bd(ep);

		/*get next BD */
		ADVANCE_BD(ep->rx_base, bd, bd_status);
		bd_status = *(u32 *) bd;
		length = (bd_status & USB_BD_LENGTH_MASK);
		ep->rx_next_bd = bd;
	}

	if (!(bd_status & R_E) && length)
		return 1;

	return 0;
}

/* Collects the submitted frame and inform the application about them.
 * It is also prepearing the TDs for new frames. If the Tx interrupt are
 * disabled, the application should call the routine to get confirmation
 * about the submitted frames. Otherwise, the routine is called from the
 * interrupt service routine during the Tx interrupt. In that case the 
 * application is informed by calling the application specific 
 * 'transaction_confirm' routine
 */
void tx_conf_interrupt(struct fhci_usb *usb)
{
	struct endpoint *ep = NULL;
	struct packet *pkt = NULL;
	struct usb_bd *bd;
	u8 *buf;
	u32 bd_status;
	int ret = TRANS_OK;

	ep = usb->ep0;

	/*collects transmitted BDs from the chip. The routine clears all BDs
	 *with R bit = 0 and the pointer to data buffer is not NULL, that is
	 *BDs which point to the transmitted data buffer
	 */
	bd = ep->tx_conf_bd;
	bd_status = *(u32 *) bd;
	buf = (u8 *) bd->buf;
	while (!(bd_status & T_R) && ((bd_status & ~T_W) || buf)) {
		/*check if it is a dummy buffer */
		if (buf == (u8 *) DUMMY_BD_BUFFER)
			break;
		else if (buf == (u8 *) DUMMY2_BD_BUFFER) {
			continue;
		}

		if (ret == TRANS_INPROGRESS)
			receive_packet_interrupt(usb->fhci);

		pkt = cq_get(ep->conf_frame_Q);

		if (bd_status & HOST_T_ERROR) {
			if (bd_status & T_NAK)
				pkt->status = USB_TD_TX_ER_NAK;
			else if (bd_status & T_TO)
				pkt->status = USB_TD_TX_ER_TIMEOUT;
			else if (bd_status & T_UN)
				pkt->status = USB_TD_TX_ER_UNDERUN;
			else if (bd_status & T_STAL)
				pkt->status = USB_TD_TX_ER_STALL;
			else
				printk("illegal error occured\n");
		}

		/*Mark BD as empty */
		*(u32 *) bd = (bd_status & T_W);
		bd->buf = 0;

		if (pkt->info & PKT_TOKEN_FRAME) {
			ret = token_confirmation_callback(usb, pkt, bd,
							  (u32) (bd_status &
								 HOST_T_ERROR));
			if (ret == TRANS_DISCARD) {
				/*advance the BD pointer */
				ADVANCE_BD(ep->tx_base, bd, bd_status);

				/*confirm BD holds the pointer to the next BD
				 *that should be transmitted*/
				ep->tx_conf_bd = bd;
			} else if (ret == TRANS_FAIL) {
				flush_all_transmissions(usb);
				return;
			}
		} else {
			if (pkt->info & PKT_ZLP)
				pkt->len = 0;
			ret = data_confirmation_callback(usb, pkt, bd,
							 (bd_status &
							  HOST_T_ERROR));
			if (ret == TRANS_FAIL) {
				flush_all_transmissions(usb);
				return;
			}
		}

		/*advance the BD pointer */
		ADVANCE_BD(ep->tx_base, bd, bd_status);

		/*confirm BD holds the pointer to the next BD 
		 *that should be transmitted*/
		ep->tx_conf_bd = bd;

		if (bd_status & HOST_T_ERROR) {
			/*issue restart Tx command */
			qe_usb_restart_tx(EP_ZERO);
			usb->fhci->regs->usb_comm =
			    (u8) (USB_CMD_STR_FIFO | EP_ZERO);
		}

		bd_status = *(u32 *) bd;
		buf = (u8 *) bd->buf;
	}
	/*schedule another transaction to this frame only if 
	 *we have already confirmed all transaction in the frame*/
	if (((get_sof_timer_count(usb) < usb->max_frame_usage) ||
	     (usb->actual_frame->frame_status & FRAME_END_TRANSMISSION)) &&
	    (list_empty(&usb->actual_frame->tds_list)))
		schedule_transactions(usb);
}

/* reset the Tx BD ring */
void flush_bds(struct fhci_usb *usb)
{
	u16 num_of_frames;
	struct packet *pkt;
	struct usb_bd *bd;
	int i;
	struct endpoint *ep = usb->ep0;
	u32 bd_status;
	u32 buff;

	num_of_frames = cq_howmany(ep->conf_frame_Q);
	for (; num_of_frames; num_of_frames--) {
		/*get next frame */
		pkt = (struct packet *)cq_get(ep->conf_frame_Q);
		if (((pkt->info & (PKT_TOKEN_FRAME | PKT_IN_TOKEN_FRAME)) ==
		     (PKT_TOKEN_FRAME | PKT_IN_TOKEN_FRAME)) ||
		    ((pkt->info & (PKT_HOST_DATA | PKT_HOST_COMMAND)) ==
		     (PKT_HOST_DATA | PKT_HOST_COMMAND)))
			*(u32 *) (pkt->priv_data) =
			    (u32) (((*(u32 *) (pkt->priv_data)) & ~T_R) |
				   T_TO);
		cq_put(ep->conf_frame_Q, pkt);
	}

	/*Fixed by Jerry huang */
	bd = (struct usb_bd *)qe_muram_addr(ep->ep_pram_ptr->tx_bd_ptr);
	bd_status = *(u32 *) bd;
	buff = bd->buf;
	do {
		if (bd_status & T_R)
			*(u32 *) bd = (u32) ((bd_status & ~T_R) | T_TO);
		else {
			bd->buf = 0;
			ep->already_pushed_dummy_bd = false;
			break;
		}

		/*advance the next BD pointer */
		ADVANCE_BD(ep->tx_base, bd, bd_status);
		bd_status = *(u32 *) bd;
		buff = bd->buf;
	} while ((bd_status & T_R) || buff);

	/*initialize Rx bds */
	bd = ep->rx_base;
	for (i = 0; i < ep->bd_ring_len; i++) {
		bd->buf = 0;
		*(u32 *) bd = (R_E | R_I);
		bd++;
	}
	bd--;
	*(u32 *) bd = (R_I | R_E | R_W);	/*for last BD set Wrap bit */

	tx_conf_interrupt(usb);

	bd = ep->tx_base;
	do {
		*(u32 *) bd = 0;
		bd->buf = 0;
		bd++;
	} while (!((*(u32 *) bd) & T_W));
	*(u32 *) bd = T_W;	/*for last BD set Wrap bit */
	bd->buf = 0;

	ep->ep_pram_ptr->rx_bd_ptr = ep->ep_pram_ptr->rx_base;
	ep->ep_pram_ptr->tx_bd_ptr = ep->ep_pram_ptr->tx_base;
	ep->ep_pram_ptr->tx_state = 0;
	ep->ep_pram_ptr->tx_cnt = 0;
	ep->tx_conf_bd = ep->tx_empty_bd = ep->tx_base;
	ep->rx_next_bd = ep->rx_empty_bd = ep->rx_base;

	while (!list_empty(&ep->active_transactions_list)) {
		struct transaction *trans = peek_transaction(ep);

		if (trans->token != NULL) {
			/*recycle the token boject */
			en_queue(&(trans->token->node),
				 &(ep->empty_tokens_list));
			trans->token = NULL;
		}
		if (trans->token_frame != NULL) {
			recycle_frame(usb, trans->token_frame);
			trans->token_frame = NULL;
		}
		if (trans->data_frame != NULL)
			trans->data_frame = NULL;

		dequeue_recycle_transaction(ep);
	}
	ep->conf_transaction = NULL;
	while (true) {
		if ((pkt = cq_get(ep->conf_frame_Q)) != NULL)
			recycle_frame(usb, pkt);
		else
			break;
	}
}
