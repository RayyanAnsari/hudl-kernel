/*
 * Copyright (C) 2012 ROCKCHIP, Inc.
 *
 * author: hhb@rock-chips.com
 * creat date: 2012-04-19
 * route:drivers/video/display/screen/lcd_hj050na_06a.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/rk_fb.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <mach/board.h>
#include <linux/rk_screen.h>
#include "../transmitter/mipi_dsi.h"

/* Base */
#define OUT_TYPE	    SCREEN_RGB

#define OUT_FACE	    OUT_P888

#define OUT_CLK	         88000000//70000000    
#define LCDC_ACLK        300000000   //29 lcdc axi DMA

/* Timing */
#define H_PW			36
#define H_BP			58
#define H_VD			900
#define H_FP			60

#define V_PW			6
#define V_BP			10
#define V_VD			1440
#define V_FP			4

#define LCD_WIDTH       94    //uint mm the lenth of lcd active area
#define LCD_HEIGHT      151
/* Other */
#define DCLK_POL		1
#define SWAP_RB			0

#define mipi_dsi_init(data) 				dsi_set_regs(data, ARRAY_SIZE(data))
#define mipi_dsi_send_dcs_packet(data) 		dsi_send_dcs_packet(data, ARRAY_SIZE(data))
#define mipi_dsi_post_init(data)			dsi_set_regs(data, ARRAY_SIZE(data))
#define data_lane  4

static struct rk29lcd_info *gLcd_info = NULL;
int lcd_init(void);
int lcd_standby(u8 enable);


static unsigned int pre_initialize[] = {

	0x00B10000 | ((V_PW & 0Xff) << 8) | (H_PW & 0Xff),
	0x00B20000 | (((V_BP+V_PW) & 0Xff) << 8) | ((H_BP+H_PW) & 0Xff),
	//0x00B20000 | ((V_BP & 0Xff) << 8) | (H_BP & 0Xff),
	0x00B30000 | ((V_FP & 0Xff) << 8) | (H_FP & 0Xff),
	0x00B40000 | H_VD,
	0x00B50000 | V_VD,
	0x00B60000 | (VPF_24BPP) | (VM_BM << 2),     //24bits
	
//	0x00c90101,
//	0x00ca0101,
	0x00de0003,    //4 lanes
	0x00d60008,
	0x00B90000,
	0x00bac01a,   //pll
	0x00Bb0006,
	0x00B70343,
	0x00B90001,
	0x00c40001,
		
};

static unsigned int post_initialize[] = {

	0x00B90000,
	
//	0x00ba8006,   //pll
//	0x00Bb0002,	
	0x00B7034b,
	
	0x00B90001,
	
	0x00c00100,      //software reset ssd2828

};


static unsigned char dcs_exit_sleep_mode[] = {0x11};
static unsigned char dcs_set_diaplay_on[] = {0x29};
static unsigned char dcs_enter_sleep_mode[] = {0x10};
static unsigned char dcs_set_diaplay_off[] = {0x28};
int ssd2828_get_status(void);
int lcd_init(void)
{
	msleep(10);

    dsi_probe_current_chip();
    if(gLcd_info)
       gLcd_info->io_init();

   	mipi_dsi_init(pre_initialize);

	mipi_dsi_send_dcs_packet(dcs_exit_sleep_mode);
	msleep(100);
	mipi_dsi_send_dcs_packet(dcs_set_diaplay_on);
	msleep(1);
	mipi_dsi_post_init(post_initialize);   

    return 0;

}



int lcd_standby(u8 enable)
{
    static int standby_status=0;
    if(enable != standby_status)
    {
    	if(enable) {
    		printk("lcd_standby...\n");
		mipi_dsi_send_dcs_packet(dcs_set_diaplay_off);
		msleep(2);		
		mipi_dsi_send_dcs_packet(dcs_enter_sleep_mode);
		msleep(100);
    		dsi_power_off();
		
    	} else {
		dsi_power_up();
    		lcd_init();
    	}
    }
    standby_status = enable;

    return 0;
}

void set_lcd_info(struct rk29fb_screen *screen, struct rk29lcd_info *lcd_info )
{
    /* screen type & face */
    screen->type = OUT_TYPE;
    screen->face = OUT_FACE;

    /* Screen size */
    screen->x_res = H_VD;
    screen->y_res = V_VD;

    screen->width = LCD_WIDTH;
    screen->height = LCD_HEIGHT;

    /* Timing */
    screen->lcdc_aclk = LCDC_ACLK;
    screen->pixclock = OUT_CLK;
	screen->left_margin = H_BP;
	screen->right_margin = H_FP;
	screen->hsync_len = H_PW;
	screen->upper_margin = V_BP;
	screen->lower_margin = V_FP;
	screen->vsync_len = V_PW;

	/* Pin polarity */
	screen->pin_hsync = 0;
	screen->pin_vsync = 0;
	screen->pin_den = 0;
	screen->pin_dclk = DCLK_POL;

	/* Swap rule */
    screen->swap_rb = SWAP_RB;
    screen->swap_rg = 0;
    screen->swap_gb = 0;
    screen->swap_delta = 0;
    screen->swap_dumy = 0;

    /* Operation function*/
    screen->init = lcd_init;
    screen->standby = lcd_standby;

    if(lcd_info)
        gLcd_info = lcd_info;
 
}

size_t get_fb_size(void)
{
	size_t size = 0;
	#if defined(CONFIG_THREE_FB_BUFFER)
		size = (((H_VD + 31)&(~31))*(V_VD)<<2)* 3; //three buffer
	#else
		size = (((H_VD + 31)&(~31))*(V_VD)<<2)<<1; //two buffer
	#endif
	return ALIGN(size,SZ_1M);
}

