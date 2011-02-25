#ifndef __POWERPC_PLATFORMS_44X_44X_H
#define __POWERPC_PLATFORMS_44X_44X_H
#include <asm/pci-bridge.h>

/* PCI support */
#define PPC4xx_PCI_CFGA_OFFSET		0
#define PPC4xx_PCI_CFGD_OFFSET		0x4

#define PPC4xx_PCIL0_PMM0LA		0x000
#define PPC4xx_PCIL0_PMM0MA		0x004
#define PPC4xx_PCIL0_PMM0PCILA		0x008
#define PPC4xx_PCIL0_PMM0PCIHA		0x00C
#define PPC4xx_PCIL0_PMM1LA		0x010
#define PPC4xx_PCIL0_PMM1MA		0x014
#define PPC4xx_PCIL0_PMM1PCILA		0x018
#define PPC4xx_PCIL0_PMM1PCIHA		0x01C
#define PPC4xx_PCIL0_PMM2LA		0x020
#define PPC4xx_PCIL0_PMM2MA		0x024
#define PPC4xx_PCIL0_PMM2PCILA		0x028
#define PPC4xx_PCIL0_PMM2PCIHA		0x02C
#define PPC4xx_PCIL0_PTM1MS		0x030
#define PPC4xx_PCIL0_PTM1LA		0x034
#define PPC4xx_PCIL0_PTM2MS		0x038
#define PPC4xx_PCIL0_PTM2LA		0x03C

extern u8 as1_readb(volatile u8 __iomem  *addr);
extern void as1_writeb(u8 data, volatile u8 __iomem *addr);

#ifdef CONFIG_PCI
extern int ppc4xx_exclude_device(struct pci_controller *hose,
				u_char bus, u_char devfn);
extern int ppc4xx_add_bridge(struct device_node *dev);
extern void ppc4xx_fixup_irq(struct pci_dev *dev);
#endif

#endif /* __POWERPC_PLATFORMS_44X_44X_H */
