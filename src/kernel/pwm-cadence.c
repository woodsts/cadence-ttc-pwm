/* pwm-cadence.c
 *
 * PWM driver for Cadence Triple Timer Counter (TTC) IPs
 *
 * Copyright (C) 2015 Xiphos Systems Corporation.
 * Licensed under the GPL-2 or later.
 *
 * Author: Berke Durak <obd@xiphos.ca>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_device.h>

struct cadence_pwm {
	void __iomem *base;
};

static int cadence_pwm_probe(struct platform_device *pdev)
{
	return -1;
}

static int cadence_pwm_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id cadence_pwm_of_match[] = {
	{ .compatible = "xlnx,ps7-ttc-1.00.a" },
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
module_platform_driver(cadence_pwm_driver);

MODULE_DESCRIPTION("PWM driver for Cadence Triple Timer Counter (TTC) IPs");
MODULE_AUTHOR("Xiphos Systems Corporation");
MODULE_LICENSE("GPL");
