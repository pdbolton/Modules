#ifndef _CH7033_H_
#define _CH7033_H_

#define CH7033_DRV_NAME 			"ch7033"
#define CH7033_ID				0x5e

#if (defined(CONFIG_MACH_DEVKIT8600) || defined(CONFIG_MACH_SBC8600))
#define PLATFORM_I2C_BUS                        1
#elif (defined(CONFIG_MACH_OMAP3_DEVKIT8500) || defined(CONFIG_MACH_OMAP3_SBC8100_PLUS) || defined(CONFIG_MACH_OMAP3_SBC8530))
#define PLATFORM_I2C_BUS                        2
#else
#error "PLATFORM_I2C_BUS not found"
#endif

#define PLATFORM_I2C_ADDR                       0x76

#endif
