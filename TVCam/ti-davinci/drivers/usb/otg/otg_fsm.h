/*
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifdef DEBUG

#define DBG(fmt, args...) printk(KERN_DEBUG "j=%lu  [%s]  " fmt "\n", \
		jiffies, __FUNCTION__, ## args)

#else
#define DBG(fmt, args...)	do {} while (0)
#endif

#ifdef VERBOSE
#define VDBG		DBG
#else
#define VDBG(stuff...)	do {} while (0)
#endif

#ifdef VERBOSE
#define MPC_LOC printk(KERN_INFO "Current Location [%s]:[%d]\n", \
		__FILE__, __LINE__)
#else
#define MPC_LOC do {} while (0)
#endif

#define PROTO_UNDEF	(0)
#define PROTO_HOST	(1)
#define PROTO_GADGET	(2)

/* OTG state machine according to the OTG spec */
struct otg_fsm {
	/* Input */
	int a_bus_resume;
	int a_bus_suspend;
	int a_conn;
	int a_sess_vld;
	int a_srp_det;
	int a_vbus_vld;
	int b_bus_resume;
	int b_bus_suspend;
	int b_conn;
	int b_se0_srp;
	int b_sess_end;
	int b_sess_vld;
	int id;

	/* Internal variables */
	int a_set_b_hnp_en;
	int b_srp_done;
	int b_hnp_enable;

	/* Timeout indicator for timers */
	int a_wait_vrise_tmout;
	int a_wait_bcon_tmout;
	int a_aidl_bdis_tmout;
	int b_ase0_brst_tmout;

	/* Informative variables */
	int a_bus_drop;
	int a_bus_req;
	int a_clr_err;
	int a_suspend_req;
	int b_bus_req;

	/* Output */
	int drv_vbus;
	int loc_conn;
	int loc_sof;

	struct otg_fsm_ops *ops;
	struct otg_transceiver *transceiver;

	/* Current usb protocol used: 0:undefine; 1:host; 2:client */
	int protocol;
	spinlock_t lock;
};

struct otg_fsm_ops {
	void (*chrg_vbus) (int on);
	void (*drv_vbus) (int on);
	void (*loc_conn) (int on);
	void (*loc_sof) (int on);
	void (*start_pulse) (void);
	void (*add_timer) (void *timer);
	void (*del_timer) (void *timer);
	int (*start_host) (struct otg_fsm *fsm, int on);
	int (*start_gadget) (struct otg_fsm *fsm, int on);
};

static inline void otg_chrg_vbus(struct otg_fsm *fsm, int on)
{
	fsm->ops->chrg_vbus(on);
}

static inline void otg_drv_vbus(struct otg_fsm *fsm, int on)
{
	if (fsm->drv_vbus != on) {
		fsm->drv_vbus = on;
		fsm->ops->drv_vbus(on);
	}
}

static inline void otg_loc_conn(struct otg_fsm *fsm, int on)
{
	if (fsm->loc_conn != on) {
		fsm->loc_conn = on;
		fsm->ops->loc_conn(on);
	}
}

static inline void otg_loc_sof(struct otg_fsm *fsm, int on)
{
	if (fsm->loc_sof != on) {
		fsm->loc_sof = on;
		fsm->ops->loc_sof(on);
	}
}

static inline void otg_start_pulse(struct otg_fsm *fsm)
{
	fsm->ops->start_pulse();
}

static inline void otg_add_timer(struct otg_fsm *fsm, void *timer)
{
	fsm->ops->add_timer(timer);
}

static inline void otg_del_timer(struct otg_fsm *fsm, void *timer)
{
	fsm->ops->del_timer(timer);
}

int otg_statemachine(struct otg_fsm *fsm);
