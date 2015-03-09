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
#include <linux/pwm.h>
#include <linux/of_address.h>
#include <linux/of_device.h>

#define DRIVER_NAME "pwm-cadence"

/* Register description (from section 8.5) */

/* Controls prescaler, selects clock input, edge [UG585] */
#define CPWM_CLK_CTRL		0x0

/* Enables counter, sets mode of operation, sets up/down counting, enabled
   matching, enabled waveform output [UG585] */
#define CPWM_CNT_CTRL		0x4

/* Current counter value */
#define CPWM_COUNT_VALUE	0x18

/* To set interval value */
#define CPWM_INTERVAL_VAL	0x24

/* Three match values */
#define CPWM_MATCH_0		0x30
#define CPWM_MATCH_1		0x3c
#define CPWM_MATCH_2		0x48

/* Interrupt status */
#define CPWM_ISR		0x54

/* Interrupt enable */
#define CPWM_IER		0x60

#define CPWM_CLK_CTRL_PS_EN		1
#define CPWM_CLK_CTRL_PS_VAL_MASK	0x1e
#define CPWM_CLK_CTRL_PS_VAL_SHIFT	1
#define CPWM_CLK_CTRL_PS_DIS		16
#define CPWM_CLK_CTRL_SRC		0x20
#define CPWM_CLK_CTRL_EXT_EDGE		0x40

/* For PWM operation, we want "interval mode" where "Interval mode: The counter
increments or decrements continuously between 0 and the value of the Interval
register, with the direction of counting determined by the DEC bit of the
Counter Control register. An interval interrupt is generated when the counter
passes through zero. The corresponding match interrupt is generated when the
counter value equals one of the Match registers." [UG585] */

struct cadence_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
};

static struct cadence_pwm_chip *cadence_pwm_get(struct pwm_chip *chip)
{
	return container_of(chip, struct cadence_pwm_chip, chip);
}

static int cadence_pwm_config(struct pwm_chip *chip,
	struct pwm_device *pwm, int duty_ns, int period_ns)
{
	struct cadence_pwm_chip *cpwm = cadence_pwm_get(chip);
	int h = pwm->hwpwm;

	printk(KERN_INFO DRIVER_NAME ": configuring %p/%s(%d), %d/%d ns\n",
		cpwm, pwm->label, h, duty_ns, period_ns);
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

	cpwm = devm_kzalloc(&pdev->dev, sizeof(*cpwm), GFP_KERNEL);
	if (!cpwm)
		return -ENOMEM;

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cpwm->base = devm_ioremap_resource(&pdev->dev, r_mem);
	if (IS_ERR(cpwm->base))
		return PTR_ERR(cpwm->base);

	cpwm->chip.dev = &pdev->dev;
	cpwm->chip.ops = &cadence_pwm_ops;
	cpwm->chip.npwm = 2;
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
