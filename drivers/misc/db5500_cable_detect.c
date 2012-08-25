/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * License Terms: GNU General Public License, version 2
 * Author: Virupax Sadashivpetimath <virupax.sadashivpetimath@stericsson.com>
 *
 * Cable detect uses 15 Virtual Irq's
 * irq's are grouped depending on the irq type.
 * attached, detached and long break.
 *
 * There are 5 uarts with 3 irq types,
 * i.e each uart with 3 types of irqs.
 * irqs:
 * 0  - 4  for cable attach
 * 5  - 9  for cable detached
 * 10 - 14 for long break.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <mach/irqs-db5500.h>

#define CD_PSEL_NBR	  0x000
#define CD_PSEL_NBR_MASK	0x7

#define CD_INTERRUPT_CTRL 0x004
#define CD_INTERRUPT_S1	  0x008
#define CD_INTERRUPT_S0	  0x00C

#define CD_INTERRUPT_STATUS 0x010
#define CD_INTERRUPT_GOAWAY 0x014

#define CD_DETECT_CTRL	  0x018
#define CD_DETECT_S1	  0x01C
#define CD_DETECT_S0	  0x020

#define LONG_BREAK	0x1
#define DETATCHED	0x2
#define ATTACHED	0x4

#define LONG_BREAK_SHIFT	16
#define DETACHED_IRQ_SHIFT	8
#define ATTACHED_IRQ_SHIFT	0

#define CD_OWNER(x)		(0x040 + (x * 4))
#define CD_ATTACHED_WM(x)	(0x060 + (x * 4))
#define CD_INPUT_OVERRIDE(x)	(0x080 + (x * 4))

#define LONG_BREAK_IRQ(x)	(((0x1 << x) & 0x1F) << LONG_BREAK_SHIFT)
#define DETACHED_IRQ(x)		(((0x1 << x) & 0x1F) << DETACHED_IRQ_SHIFT)
#define ATTACHED_IRQ(x)		(((0x1 << x) & 0x1F) << ATTACHED_IRQ_SHIFT)

#define LONG_BREAK_IRQ_MASK	(0x1F << LONG_BREAK_SHIFT)
#define DETACHED_IRQ_MASK	(0x1F << DETACHED_IRQ_SHIFT)
#define ATTACHED_IRQ_MASK	(0x1F << ATTACHED_IRQ_SHIFT)

#define CD_INPUT_OVERRIDE_DEFAULT	0x3
#define CD_INPUT_OVERRIDE_EN	0x0

#define TOTAL_NUM_UARTS 5
#define MASTER_ARM_A9 1
#define ATTATCHED_IRQ_NUM 5
#define DETACHED_IRQ_NUM 5

static unsigned int cd_irq_type_shift[] = {
	ATTACHED_IRQ_SHIFT,
	DETACHED_IRQ_SHIFT,
	LONG_BREAK_SHIFT,
};

static DEFINE_SPINLOCK(cd_irq_lock);

struct cable_dt_irqs {
	unsigned long base;
	unsigned long count;
};

struct cable_dt {
	void __iomem *cd_base;
	struct device *dev;
	struct cable_dt_irqs *cd_irq;
	struct clk *clk;
	int cdt_irq;
};

#ifdef DEBUG
static void cd_print_regs(struct cable_dt *cd, const char *func_name)
{
	int i;

	dev_dbg(cd->dev, "%s\n" , func_name);

	dev_dbg(cd->dev, "INTERRUPT_CTRL 0x%x\n",
			readl(cd->cd_base + CD_INTERRUPT_CTRL));
	dev_dbg(cd->dev, "INTERRUPT_STATUS 0x%x\n",
			readl(cd->cd_base + CD_INTERRUPT_STATUS));
	dev_dbg(cd->dev, "INTERRUPT_GOAWAY 0x%x\n",
			readl(cd->cd_base + CD_INTERRUPT_GOAWAY));
	dev_dbg(cd->dev, "DETECT_CTRL 0x%x\n",
			readl(cd->cd_base + CD_DETECT_CTRL));

	for (i = 0; i < TOTAL_NUM_UARTS; i++) {
		dev_dbg(cd->dev, "CD_OWNER.%d %d\n", i,
				readl(cd->cd_base + CD_OWNER(i)));
		dev_dbg(cd->dev, "CD_ATTACHED_WM.%d %d\n", i,
				readl(cd->cd_base + CD_ATTACHED_WM(i)));
		dev_dbg(cd->dev, "CD_INPUT_OVERRIDE.%d %d\n", i,
				readl(cd->cd_base + CD_INPUT_OVERRIDE(i)));
	}
}
#else
#define cd_print_regs(x, y)
#endif

static void cd_set_watermark(struct cable_dt *cd, int watermark, int uart)
{
	writel(watermark, cd->cd_base + CD_ATTACHED_WM(uart));
}

static void cd_set_owner(struct cable_dt *cd, int master, int uart)
{
	writel(master, cd->cd_base + CD_OWNER(uart));
}

static void cd_override(struct cable_dt *cd, int uart)
{
	writel(CD_INPUT_OVERRIDE_EN, cd->cd_base + CD_INPUT_OVERRIDE(uart));
}

static void cd_un_override(struct cable_dt *cd, int uart)
{
	writel(CD_INPUT_OVERRIDE_DEFAULT, cd->cd_base +
			CD_INPUT_OVERRIDE(uart));
}

static int irq_to_type(unsigned int irq)
{
	/* calculate the type of irq */
	return	irq / TOTAL_NUM_UARTS;
}

static int irq_to_uart_num(unsigned int irq)
{
	return irq % TOTAL_NUM_UARTS;
}

/* Enable cable detect for the given uart */
static void cd_enable(struct cable_dt *cd, int cd_irq)
{
	unsigned int irq_type = irq_to_type(cd_irq);
	unsigned int uart = irq_to_uart_num(cd_irq);

	spin_lock(&cd_irq_lock);
	writel((0x1 << uart) << cd_irq_type_shift[irq_type], cd->cd_base
			+ CD_INTERRUPT_S1);
	writel(0x1 << uart, cd->cd_base + CD_DETECT_S1);
	spin_unlock(&cd_irq_lock);
}

static void cd_disable(struct cable_dt *cd, int cd_irq)
{
	unsigned int irq_type = irq_to_type(cd_irq);
	unsigned int uart = irq_to_uart_num(cd_irq);

	spin_lock(&cd_irq_lock);
	/* Disable a irqtype for a uart */
	writel((0x1 << uart) << cd_irq_type_shift[irq_type],
			cd->cd_base + CD_INTERRUPT_S0);
	spin_unlock(&cd_irq_lock);
}

static void handle_long_break_irq(struct cable_dt *cd, int uarts)
{
	int i;

	dev_dbg(cd->dev, "Cable detect: Long Break Interrupt\n");

	for (i = 0; i < TOTAL_NUM_UARTS; i++)
		if (uarts & (0x1 << i))
			handle_nested_irq(cd->cd_irq->base + i +
					ATTATCHED_IRQ_NUM + DETACHED_IRQ_NUM);
}

static void handle_detached_irq(struct cable_dt *cd, int uarts)
{
	int i;

	dev_dbg(cd->dev, "Cable detect: Detached Interrupt\n");

	for (i = 0; i < TOTAL_NUM_UARTS; i++)
		if (uarts & (0x1 << i))
			handle_nested_irq(cd->cd_irq->base + i
					+ ATTATCHED_IRQ_NUM);
}

static void handle_attached_irq(struct cable_dt *cd, int uarts)
{
	int i;

	dev_dbg(cd->dev, "Cable detect: Long Attached Interrupt\n");

	for (i = 0; i < TOTAL_NUM_UARTS; i++)
		if (uarts & (0x1 << i))
			handle_nested_irq(cd->cd_irq->base + i);
}

static irqreturn_t cd_interrupt_handler(int irq, void *dev_id)
{
	struct cable_dt *cd = (struct cable_dt *)dev_id;
	u32 status;

	clk_enable(cd->clk);
	status = readl(cd->cd_base + CD_INTERRUPT_STATUS);
	if (!status) {
		clk_disable(cd->clk);
		return IRQ_NONE;
	}

	/* Clear the interrupt */
	writel(status, cd->cd_base + CD_INTERRUPT_GOAWAY);
	clk_disable(cd->clk);

	if (status & LONG_BREAK_IRQ_MASK)
		handle_long_break_irq(cd, (status & LONG_BREAK_IRQ_MASK)
				>> LONG_BREAK_SHIFT);

	if (status & DETACHED_IRQ_MASK)
		handle_detached_irq(cd, (status & DETACHED_IRQ_MASK)
				>> DETACHED_IRQ_SHIFT);

	if (status & ATTACHED_IRQ_MASK)
		handle_attached_irq(cd, (status & ATTACHED_IRQ_MASK)
				>> ATTACHED_IRQ_SHIFT);

	return IRQ_HANDLED;
}

static void cd_enable_irq(unsigned int irq)
{
	struct cable_dt *cd = get_irq_chip_data(irq);
	int uart_num = irq_to_uart_num(irq - cd->cd_irq->base);

	/* Configure CPU as the master for the uart */
	cd_set_owner(cd, MASTER_ARM_A9, uart_num);
	/* No. of CD clock cycles after which detect event is generated */
	cd_set_watermark(cd, 0x1FF, uart_num);
	/* Enable the UART Rx line detection */
	cd_override(cd, uart_num);
	/* Enable detection for the uart */
	cd_enable(cd, irq - cd->cd_irq->base);
	cd_print_regs(cd, __func__);
}

static void cd_disable_irq(unsigned int irq)
{
	struct cable_dt *cd = get_irq_chip_data(irq);

	cd_disable(cd, irq - cd->cd_irq->base);
	cd_print_regs(cd, __func__);
}

static void cd_shutdown_irq(unsigned int irq)
{
	struct cable_dt *cd = get_irq_chip_data(irq);
	unsigned int uart = irq_to_uart_num(irq - cd->cd_irq->base);
	unsigned int uartx_irqs;
	unsigned int uartx_irqs_enabled;

	/* Possible irq's for a uart */
	uartx_irqs = LONG_BREAK_IRQ(uart) | DETACHED_IRQ(uart) |
		ATTACHED_IRQ(uart);

	/* Get the irqs enabled for a uart */
	uartx_irqs_enabled = uartx_irqs & readl(cd->cd_base +
			CD_INTERRUPT_CTRL);

	/*
	 * If no irq types are enabled for a uart
	 * then disable the irq detection for
	 * that uart.
	 */
	if (!uartx_irqs_enabled) {
		writel((0x1 << uart), cd->cd_base + CD_DETECT_S0);

		/*
		 * Disable the UART Rx line detection, restore back
		 * default values.
		 */
		cd_un_override(cd, uart);
		cd_set_watermark(cd, 0xFFFF, uart);
		cd_set_owner(cd, 0x0, uart);
	}
	cd_print_regs(cd, __func__);
}

static void cd_bus_lock(unsigned int irq)
{
	struct cable_dt *cd = get_irq_chip_data(irq);

	clk_enable(cd->clk);
}

static void cd_bus_sync_unlock(unsigned int irq)
{
	struct cable_dt *cd = get_irq_chip_data(irq);

	clk_disable(cd->clk);
}

static struct irq_chip cd_chip = {
	.name			= "Cable-Detect",
	.bus_lock		= cd_bus_lock,
	.bus_sync_unlock	= cd_bus_sync_unlock,
	.disable		= cd_disable_irq,
	.enable			= cd_enable_irq,
	.shutdown		= cd_shutdown_irq,
};

static int __devinit cd_probe(struct platform_device *pdev)
{
	struct resource	*res;
	int ret;
	int irq;
	struct cable_dt *cd;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	cd = kzalloc(sizeof(struct cable_dt), GFP_KERNEL);
	if (!cd) {
		dev_err(&pdev->dev, "cannot allocate memory\n");
		return -ENOMEM;
	}

	cd->dev = &pdev->dev;
	cd->cd_irq = pdev->dev.platform_data;
	platform_set_drvdata(pdev, cd);

	cd->clk = clk_get(&pdev->dev, "cable_detect");
	if (IS_ERR(cd->clk)) {
		dev_err(&pdev->dev, "could not get CD clock\n");
		ret = PTR_ERR(cd->clk);
		goto err_no_clk;
	}

	cd->cdt_irq = platform_get_irq(pdev, 0);
	if (cd->cdt_irq < 0) {
		ret = cd->cdt_irq;
		goto irq_get_fail;
	}

	cd->cd_base = ioremap(res->start, resource_size(res));
	if (!cd->cd_base) {
		dev_err(cd->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto ioremap_fail;
	}

	ret = request_threaded_irq(cd->cdt_irq, NULL, cd_interrupt_handler,
			IRQF_ONESHOT | IRQF_NO_SUSPEND, "cable_detect", cd);
	if (ret) {
		dev_err(cd->dev, "IRQ Get failed (%d)\n", ret);
		goto irq_req_fail;
	}

	/*
	 * Setup the cablde detect event handler.
	 */
	for (irq = cd->cd_irq->base;
			irq < (cd->cd_irq->base + cd->cd_irq->count);
			irq++) {
		set_irq_chip_data(irq, cd);
		set_irq_chip_and_handler(irq, &cd_chip,
					 handle_simple_irq);
		set_irq_nested_thread(irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(irq, IRQF_VALID);
#else
		set_irq_noprobe(irq);
#endif
	}

	return 0;

irq_req_fail:
	iounmap(cd->cd_base);
ioremap_fail:
irq_get_fail:
	clk_put(cd->clk);
err_no_clk:
	kfree(cd);
	return ret;
}

static int __devexit cd_remove(struct platform_device *pdev)
{
	int irq;
	struct cable_dt *cd = platform_get_drvdata(pdev);

	for (irq = cd->cd_irq->base;
			irq < (cd->cd_irq->base + cd->cd_irq->count);
			irq++) {
#ifdef CONFIG_ARM
		set_irq_flags(irq, 0);
#endif
		set_irq_chip_and_handler(irq, NULL, NULL);
		set_irq_chip_data(irq, NULL);
	}
	free_irq(cd->cdt_irq, cd);
	clk_put(cd->clk);
	iounmap(cd->cd_base);
	kfree(cd);

	return 0;
}

static struct platform_driver cabled_driver = {
	.driver		= {
		.name	= "cable_detect",
		.owner	= THIS_MODULE,
	},
	.probe		= cd_probe,
	.remove		= __devexit_p(cd_remove),
};

static int __init cd_init(void)
{
	return platform_driver_register(&cabled_driver);
}
subsys_initcall(cd_init);

static void __exit cd_exit(void)
{
	platform_driver_unregister(&cabled_driver);
}
module_exit(cd_exit);

MODULE_AUTHOR("Virupax Sadashivpetimath <virupax.sadashivpetimath@stericsson.com>");
MODULE_DESCRIPTION("UART Cable Detect Driver");
MODULE_LICENSE("GPL");
