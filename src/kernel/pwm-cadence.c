/* pwm-cadence.c
 *
 * PWM driver for Cadence Triple Timer Counter (TTC) IPs
 *
 * Copyright (C) 2015 Xiphos Systems Corporation.
 * Licensed under the GPL-2 or later.
 *
 * Author: Berke Durak <obd@xiphos.ca>
 *
 * Based in part on:
 *   pwm-lpc32xx.c
 *
 * References:
 *   [UG585] Zynq-7000 All Programmable SoC Technical Reference Manual, Xilinx
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <asm/div64.h>

#define DRIVER_NAME "pwm-cadence"

/* Register description (from section 8.5) */

enum cpwm_register {
	CPWM_CLK_CTRL = 0,
	CPWM_COUNTER_CTRL = 1,
	CPWM_COUNTER_VALUE = 2,
	CPWM_INTERVAL_COUNTER = 3,
	CPWM_MATCH_1_COUNTER = 4,
	CPWM_MATCH_2_COUNTER = 5,
	CPWM_MATCH_3_COUNTER = 6,
	CPWM_INTERRUPT_REGISTER = 7,
	CPWM_INTERRUPT_ENABLE = 8,
	CPWM_EVENT_CONTROL_TIMER = 9,
	CPWM_EVENT_REGISTER = 10
};

#define CPWM_CLK_FALLING_EDGE 0x40
#define CPWM_CLK_SRC_EXTERNAL 0x20
#define CPWM_CLK_PRESCALE_SHIFT 1
#define CPWM_CLK_PRESCALE_MASK (15 << 1)
#define CPWM_CLK_PRESCALE_ENABLE 1

#define CPWM_NUM_PWM 3

/* For PWM operation, we want "interval mode" where "Interval mode: The counter
increments or decrements continuously between 0 and the value of the Interval
register, with the direction of counting determined by the DEC bit of the
Counter Control register. An interval interrupt is generated when the counter
passes through zero. The corresponding match interrupt is generated when the
counter value equals one of the Match registers." [UG585] */

struct cadence_pwm_pwm {
	uint32_t clk_hz; /* Per-PWM clock frequency */
	int source;      /* Per-PWM source */
};

struct cadence_pwm_chip {
	struct pwm_chip chip;
	char __iomem *base;
	struct cadence_pwm_pwm pwms[CPWM_NUM_PWM];
};

static inline struct cadence_pwm_chip *cadence_pwm_get(struct pwm_chip *chip)
{
	return container_of(chip, struct cadence_pwm_chip, chip);
}

static inline volatile __iomem uint32_t *cpwm_register_address(
	struct cadence_pwm_chip *cpwm, int pwm, enum cpwm_register reg)
{
	return (uint32_t *) (4 * (3 * reg + pwm) +
		(char *) cpwm->base);
}

static uint32_t cpwm_read(struct cadence_pwm_chip *cpwm, int pwm,
	enum cpwm_register reg)
{
	uint32_t x;

	x = ioread32(cpwm_register_address(cpwm, pwm, reg));
	printk(KERN_DEBUG "Read %08x from %p:%d register %d\n", x, cpwm, pwm,
		reg);
	return x;
}

static void cpwm_write(struct cadence_pwm_chip *cpwm, int pwm,
	enum cpwm_register reg, uint32_t value)
{
	printk(KERN_DEBUG "Write %08x to %p:%d register %d\n", value, cpwm, pwm,
		reg);
	iowrite32(value, cpwm_register_address(cpwm, pwm, reg));
}

static int cadence_pwm_config(struct pwm_chip *chip,
	struct pwm_device *pwm, int duty_ns, int period_ns)
{
	struct cadence_pwm_chip *cpwm = cadence_pwm_get(chip);
	int h = pwm->hwpwm;
	uint32_t x;
	int period_clocks, prescaler;

	printk(KERN_INFO DRIVER_NAME ": configuring %p/%s(%d), %d/%d ns\n",
		cpwm, pwm->label, h, duty_ns, period_ns);

	if (period_ns < 0)
		return -EINVAL;

	period_clocks =
		div64_u64(
			((int64_t) period_ns * (int64_t) cpwm->pwms[h].clk_hz),
			1000000000LL);
	
	prescaler = ilog2(period_clocks) + 1 - 16;
	if (prescaler < 0) prescaler = 0;

	printk(KERN_INFO DRIVER_NAME ": %d clocks, prescaler 2^%d\n",
		period_clocks, prescaler);

	x = cpwm_read(cpwm, h, CPWM_CLK_CTRL);

	x &= ~CPWM_CLK_PRESCALE_MASK;
	x |= (prescaler << CPWM_CLK_PRESCALE_SHIFT) & CPWM_CLK_PRESCALE_MASK;

	if (cpwm->pwms[h].source) x |= CPWM_CLK_SRC_EXTERNAL;
	else x &= ~CPWM_CLK_SRC_EXTERNAL;

	cpwm_write(cpwm, h, CPWM_CLK_CTRL, x);

	return 0;
}

static void cadence_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	printk(KERN_INFO DRIVER_NAME ": disabling %p\n", chip);
}

static int cadence_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	printk(KERN_INFO DRIVER_NAME ": enabling %p\n", chip);
	return 0;
}

static const struct pwm_ops cadence_pwm_ops = {
	.config = cadence_pwm_config,
	.enable = cadence_pwm_enable,
	.disable = cadence_pwm_disable,
	.owner = THIS_MODULE,
};

static int cadence_pwm_probe(struct platform_device *pdev)
{
	struct cadence_pwm_chip *cpwm;
	struct resource *r_mem;
	int ret;
	struct device_node *node = pdev->dev.of_node;
	const __be32 *value;
	int rlen;
	char propname[24];
	int i;
	struct cadence_pwm_pwm *pwm;

	cpwm = devm_kzalloc(&pdev->dev, sizeof(*cpwm), GFP_KERNEL);
	if (!cpwm)
		return -ENOMEM;

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cpwm->base = devm_ioremap_resource(&pdev->dev, r_mem);
	if (IS_ERR(cpwm->base))
		return PTR_ERR(cpwm->base);

	for (i = 0; i < CPWM_NUM_PWM; i ++) {
		pwm = cpwm->pwms + i;

		snprintf(propname, sizeof(propname),
			"xlnx,ttc-clk%d-freq-hz", i);

		value = of_get_property(node, propname, &rlen);
		if (value)
			pwm->clk_hz = be32_to_cpup(value);
		else {
			dev_err(&pdev->dev, "missing %s property", propname);
			return -ENODEV;
		}

		snprintf(propname, sizeof(propname),
			"xlnx,ttc-clk%d-clksrc", i);

		value = of_get_property(node, propname, &rlen);
		if (value)
			pwm->source = be32_to_cpup(value);
		else {
			dev_err(&pdev->dev, "missing %s property", propname);
			return -ENODEV;
		}

		dev_info(&pdev->dev, "PWM %d has clock source %d at %d Hz",
				i, pwm->source, pwm->clk_hz);
	}

	cpwm->chip.dev = &pdev->dev;
	cpwm->chip.ops = &cadence_pwm_ops;
	cpwm->chip.npwm = CPWM_NUM_PWM;
	cpwm->chip.base = -1;

	ret = pwmchip_add(&cpwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "cannot add pwm chip (error %d)", ret);
		return ret;
	}

	platform_set_drvdata(pdev, cpwm);
	return 0;
}

static int cadence_pwm_remove(struct platform_device *pdev)
{
	struct cadence_pwm_chip *cpwm = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < cpwm->chip.npwm; i ++)
		pwm_disable(&cpwm->chip.pwms[i]);

	return pwmchip_remove(&cpwm->chip);
}

static const struct of_device_id cadence_pwm_of_match[] = {
	{ .compatible = "xlnx,ps7-ttc-1.00.a" },
	{ .compatible = "cdns,ttc" },
	{},
};

MODULE_DEVICE_TABLE(of, cadence_pwm_of_match);

static struct platform_driver cadence_pwm_driver = {
	.driver = {
		.name = "pwm-cadence",
		.owner = THIS_MODULE,
		.of_match_table = cadence_pwm_of_match,
	},
	.probe = cadence_pwm_probe,
	.remove = cadence_pwm_remove,
};

static int __init cadence_pwm_init(void)
{
	int ret;

	printk(KERN_INFO "cadence_pwm: Compiled on "
		__DATE__ " at " __TIME__ "\n");
	ret = platform_driver_register(&cadence_pwm_driver);
	return ret;
}

static void __exit cadence_pwm_exit(void)
{
	platform_driver_unregister(&cadence_pwm_driver);
}

module_init(cadence_pwm_init);
module_exit(cadence_pwm_exit);

MODULE_DESCRIPTION("PWM driver for Cadence Triple Timer Counter (TTC) IPs");
MODULE_AUTHOR("Xiphos Systems Corporation");
MODULE_LICENSE("GPL");
