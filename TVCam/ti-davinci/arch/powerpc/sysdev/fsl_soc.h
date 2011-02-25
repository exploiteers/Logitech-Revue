#ifndef __PPC_FSL_SOC_H
#define __PPC_FSL_SOC_H
#ifdef __KERNEL__

#include <asm/mmu.h>
#include <asm/prom.h>

extern phys_addr_t immrbase;
extern u32 brgfreq;
extern u32 fs_baudrate;

extern phys_addr_t get_immrbase(void);
extern u32 get_brgfreq(void);
extern u32 get_baudrate(void);
extern u32 fsl_get_sys_freq(void);

struct dsk {
	char *dev;
	char *prm;
	u32 *result;
};

static inline int early_get_from_flat_dt(unsigned long node, const char *uname,
					int depth, void *data)
{
	struct dsk *p = data;
	char *type = of_get_flat_dt_prop(node, "device_type", NULL);
	u32 *param;
	unsigned long len;

	if (type == NULL || strcmp(type, p->dev) != 0)
		return 0;

	param = of_get_flat_dt_prop(node, p->prm, &len);
	*(p->result) = *param;
	return 1;
}

static inline void get_from_flat_dt(char *device, char *param, void *save_in)
{
	struct dsk pack = { device, param, save_in };
	of_scan_flat_dt(early_get_from_flat_dt, &pack);
}

struct spi_board_info;

extern int fsl_spi_init(struct spi_board_info *board_infos,
			unsigned int num_board_infos,
			void (*activate_cs)(u8 cs, u8 polarity),
			void (*deactivate_cs)(u8 cs, u8 polarity));

extern void fsl_rstcr_restart(char *cmd);

#if defined(CONFIG_FB_FSL_DIU) || defined(CONFIG_FB_FSL_DIU_MODULE)
#include <linux/bootmem.h>
#include <asm/rheap.h>
struct platform_diu_data_ops {
	rh_block_t diu_rh_block[16];
	rh_info_t diu_rh_info;
	unsigned long diu_size;
	void *diu_mem;

	unsigned int (*get_pixel_format) (unsigned int bits_per_pixel,
		int monitor_port);
	void (*set_gamma_table) (int monitor_port, char *gamma_table_base);
	void (*set_monitor_port) (int monitor_port);
	void (*set_pixel_clock) (unsigned int pixclock);
	ssize_t (*show_monitor_port) (int monitor_port, char *buf);
	int (*set_sysfs_monitor_port) (int val);
};

extern struct platform_diu_data_ops diu_ops;
int __init preallocate_diu_videomemory(void);
#endif

#endif
#endif
