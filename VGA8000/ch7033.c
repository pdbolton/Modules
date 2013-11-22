/*
 *  mma7660.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2009-2010 Freescale Semiconductor Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>

#include "ch7033.h"

//Chrontel type define: 
typedef unsigned long long	ch_uint64;
typedef unsigned int		ch_uint32;
typedef unsigned short		ch_uint16;
typedef unsigned char		ch_uint8;
typedef ch_uint32		ch_bool;
#define ch_true			1
#define ch_false		0

/*
 * Defines
 */
#if 0
#define assert(expr)\
        if(!(expr)) {\
        printk( "Assertion failed! %s,%d,%s,%s\n",\
        __FILE__,__LINE__,__func__,#expr);\
        }
#else
#define assert(expr) do{} while(0)
#endif

#define CH7033_DRV_NAME	"ch7033"

/**********************************************************************/
/*                                                                    */
/* Description: CH7033/34/35 set up code, realized by C code          */
/*                                                                    */
/*              1. Integrate this file to your project                */
/*                                                                    */
/*              2. Add realization of IIC read/write function         */
/*                                                                    */
/*              3. Call InitializeCH703X                              */
/*                                                                    */
/* Create By: CH7033/34/35 Program Tool Rev1.02.02                    */
/*                                                                    */
/* Create Date: 2011-07-25                                            */
/*                                                                    */
/**********************************************************************/

static struct i2c_client *g_client;

//IIC function: read/write CH7033
ch_uint32 I2CRead(ch_uint32 index)
{
	unsigned int result;
	//Realize this according to your system...
        result = i2c_smbus_read_byte_data(g_client, index);
	return result;
}
void I2CWrite(ch_uint32 index, ch_uint32 value)
{
	int result;
	//Realize this according to your system...
	result = i2c_smbus_write_byte_data(g_client, index, value);
	assert(result==0);
}

//1280x1024 vga timming
static ch_uint32 CH7033_VGA_RegTable[][2] = {
	{ 0x03, 0x04 },
	{ 0x52, 0x01 },
	{ 0x52, 0x03 },
	{ 0x03, 0x00 },
	{ 0x07, 0xD9 },
	{ 0x08, 0xF1 },
	{ 0x09, 0x03 },
	{ 0x0B, 0x2C },
	{ 0x0C, 0x00 },
	{ 0x0D, 0x3F },
	{ 0x0F, 0x12 },
	{ 0x10, 0x35 },
	{ 0x11, 0x1B },
	{ 0x12, 0x00 },
	{ 0x13, 0x26 },
	{ 0x15, 0x03 },
	{ 0x16, 0x06 },
	{ 0x19, 0xC9 },
	{ 0x1A, 0x19 },
	{ 0x1B, 0x40 },
	{ 0x1C, 0x69 },
	{ 0x1D, 0x78 },
	{ 0x1E, 0x80 },
	{ 0x1F, 0x35 },
	{ 0x20, 0x00 },
	{ 0x21, 0x98 },
	{ 0x25, 0x24 },
	{ 0x26, 0x00 },
	{ 0x27, 0x2A },
	{ 0x2E, 0x3D },
	{ 0x55, 0x30 },
	{ 0x56, 0x70 },
	{ 0x58, 0x01 },
	{ 0x59, 0x03 },
	{ 0x5A, 0x04 },
	{ 0x5E, 0x54 },
	{ 0x64, 0x30 },
	{ 0x68, 0x46 },
	{ 0x6A, 0x45 },
	{ 0x6B, 0x90 },
	{ 0x6D, 0x7C },
	{ 0x6E, 0xAE },
	{ 0x74, 0x30 },
	{ 0x03, 0x01 },
	{ 0x08, 0x05 },
	{ 0x12, 0xD6 },
	{ 0x13, 0x28 },
	{ 0x15, 0x00 },
	{ 0x23, 0xE3 },
	{ 0x28, 0x54 },
	{ 0x29, 0x60 },
	{ 0x6B, 0x11 },
	{ 0x03, 0x03 },
	{ 0x03, 0x04 },
	{ 0x10, 0x01 },
	{ 0x11, 0xA5 },
	{ 0x12, 0xE0 },
	{ 0x13, 0x02 },
	{ 0x14, 0x88 },
	{ 0x15, 0x70 },
	{ 0x54, 0xC4 },
	{ 0x55, 0x5B },
	{ 0x56, 0x4D },
	{ 0x61, 0x60 },
	{ 0x7F, 0xFE },
};
#define REGTABLE_VGA_LEN	((sizeof(CH7033_VGA_RegTable))/(2*sizeof(ch_uint32)))

ch_bool InitializeCH7033(void)
{
	ch_uint32 i;
	ch_uint32 val_t;
	ch_uint32 hinc_reg, hinca_reg, hincb_reg;
	ch_uint32 vinc_reg, vinca_reg, vincb_reg;
	ch_uint32 hdinc_reg, hdinca_reg, hdincb_reg;

	for(i=0; i<REGTABLE_VGA_LEN; ++i)
	{
		I2CWrite(CH7033_VGA_RegTable[i][0], CH7033_VGA_RegTable[i][1]);
	}

	//2. Calculate online parameters:
	I2CWrite(0x03, 0x00);
	i = I2CRead(0x25);
	I2CWrite(0x03, 0x04);
	//HINCA:
	val_t = I2CRead(0x2A);
	hinca_reg = (val_t << 3) | (I2CRead(0x2B) & 0x07);
	//HINCB:
	val_t = I2CRead(0x2C);
	hincb_reg = (val_t << 3) | (I2CRead(0x2D) & 0x07);
	//VINCA:
	val_t = I2CRead(0x2E);
	vinca_reg = (val_t << 3) | (I2CRead(0x2F) & 0x07);
	//VINCB:
	val_t = I2CRead(0x30);
	vincb_reg = (val_t << 3) | (I2CRead(0x31) & 0x07);
	//HDINCA:
	val_t = I2CRead(0x32);
	hdinca_reg = (val_t << 3) | (I2CRead(0x33) & 0x07);
	//HDINCB:
	val_t = I2CRead(0x34);
	hdincb_reg = (val_t << 3) | (I2CRead(0x35) & 0x07);
	//no calculate hdinc if down sample disaled
	if(i & (1 << 6))
	{
		if(hdincb_reg == 0)
		{
			return ch_false;
		}
		//hdinc_reg = (ch_uint32)(((ch_uint64)hdinca_reg) * (1 << 20) / hdincb_reg);
		hdinc_reg = (ch_uint32)div_u64(((ch_uint64)hdinca_reg) * (1 << 20), hdincb_reg);
		I2CWrite(0x3C, (hdinc_reg >> 16) & 0xFF);
		I2CWrite(0x3D, (hdinc_reg >>  8) & 0xFF);
		I2CWrite(0x3E, (hdinc_reg >>  0) & 0xFF);
	}
	if(hincb_reg == 0 || vincb_reg == 0)
	{
		return ch_false;
	}
	if(hinca_reg > hincb_reg)
	{
		return ch_false;
	}
	//hinc_reg = (ch_uint32)((ch_uint64)hinca_reg * (1 << 20) / hincb_reg);
	hinc_reg = (ch_uint32)div_u64((ch_uint64)hinca_reg * (1 << 20), hincb_reg);
	//vinc_reg = (ch_uint32)((ch_uint64)vinca_reg * (1 << 20) / vincb_reg);
	vinc_reg = (ch_uint32)div_u64((ch_uint64)vinca_reg * (1 << 20), vincb_reg);
	I2CWrite(0x36, (hinc_reg >> 16) & 0xFF);
	I2CWrite(0x37, (hinc_reg >>  8) & 0xFF);
	I2CWrite(0x38, (hinc_reg >>  0) & 0xFF);
	I2CWrite(0x39, (vinc_reg >> 16) & 0xFF);
	I2CWrite(0x3A, (vinc_reg >>  8) & 0xFF);
	I2CWrite(0x3B, (vinc_reg >>  0) & 0xFF);

	//3. Start to running:
	I2CWrite(0x03, 0x00);
	val_t = I2CRead(0x0A);
	I2CWrite(0x0A, val_t | 0x80);
	I2CWrite(0x0A, val_t & 0x7F);
	val_t = I2CRead(0x0A);
	I2CWrite(0x0A, val_t & 0xEF);
	I2CWrite(0x0A, val_t | 0x10);
	I2CWrite(0x0A, val_t & 0xEF);

	return ch_true;
}

/*
 * Initialization function
 */


/*
 * I2C init/probing/exit functions
 */
static int __devinit ch7033_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);

	g_client = client;

/*	printk(KERN_INFO "ch7033_probe\n");*/
	result = i2c_check_functionality(adapter, 
		I2C_FUNC_SMBUS_BYTE|I2C_FUNC_SMBUS_BYTE_DATA);
	assert(result);

	
	result = i2c_smbus_write_byte_data(client, 0x3, 0x4);
                assert(result==0);

        result = i2c_smbus_read_byte_data(client, 0x50);
	printk(KERN_INFO "id= 0x%x\n", result);
	if (result != CH7033_ID) {
		printk(KERN_ERR "ch7033 not found!\n");
		return -ENODEV;
	}

	InitializeCH7033();

	return 0;
}

static int __devexit ch7033_remove(struct i2c_client *client)
{
	return 0;
}

#ifdef CONFIG_PM

static int ch7033_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int ch7033_resume(struct i2c_client *client)
{
	return 0;
}

#else

#define ch7033_suspend		NULL
#define ch7033_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id ch7033_id[] = {
	{ CH7033_DRV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ch7033_id);

static struct i2c_driver ch7033_driver = {
	.driver = {
		.name	= CH7033_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = ch7033_suspend,
	.resume	= ch7033_resume,
	.probe	= ch7033_probe,
	.remove	= __devexit_p(ch7033_remove),
	.id_table = ch7033_id,
};

static struct i2c_board_info i2c_board_info[] = {
        {
                I2C_BOARD_INFO(CH7033_DRV_NAME, PLATFORM_I2C_ADDR),
                .platform_data = NULL,
        },
};

static int __init ch7033_init(void)
{
        struct i2c_adapter *adapter;
        struct i2c_client *client;

        adapter = i2c_get_adapter(PLATFORM_I2C_BUS);
        if (!adapter) {
                printk("Unable to get i2c adapter on bus %d.\n", PLATFORM_I2C_BUS);
                return -ENODEV;
        }

        client = i2c_new_device(adapter, i2c_board_info);
        i2c_put_adapter(adapter);
        if (!client) {
                printk("Unable to create i2c device on bus %d.\n", PLATFORM_I2C_BUS);
                return -ENODEV;
        }

	return i2c_add_driver(&ch7033_driver);
}

static void __exit ch7033_exit(void)
{
	i2c_del_driver(&ch7033_driver);
	i2c_unregister_device(g_client);
}

MODULE_AUTHOR("fourier <samssmarm@gmail.com>");
MODULE_DESCRIPTION("ch7033 VGA/HDMI driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(ch7033_init);
module_exit(ch7033_exit);
