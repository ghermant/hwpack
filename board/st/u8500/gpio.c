/*
 * (C) Copyright 2009
 * STEricsson, <www.stericsson.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/* --- includes ----------------------------------------------------------- */

#include "gpio.h"

static	struct gpio_register *addr_gpio_register[GPIO_BLOCKS_COUNT];

int sz_altfun_tbl;

struct gpio_altfun_data altfun_table[] = {
    {.altfun = GPIO_ALT_UART_0_MODEM,.start = 0,.end = 3,.cont = 1,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_UART_0_MODEM,.start = 33,.end = 36,.cont = 0,.type =
        GPIO_ALTF_C,},        
    {.altfun = GPIO_ALT_UART_1,.start = 4,.end = 7,.cont = 0,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_UART_2,.start = 18,.end = 19,.cont = 1,.type =
        GPIO_ALTF_B,},
    {.altfun = GPIO_ALT_UART_2,.start = 29,.end = 32,.cont = 0,.type =
        GPIO_ALTF_C,},
    {.altfun = GPIO_ALT_MSP_0,.start = 12,.end = 17,.cont = 1,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_MSP_0,.start = 21,.end = 21,.cont = 0,.type =
        GPIO_ALTF_B,},
    {.altfun = GPIO_ALT_MSP_1,.start = 33,.end = 36,.cont = 0,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_MSP_2,.start = 192,.end = 196,.cont = 0,.type =
        GPIO_ALTF_A,}, 
    {.altfun = GPIO_ALT_LCD_PANEL,.start = 64,.end = 93,.cont = 1,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_LCD_PANEL,.start = 150,.end = 171,.cont = 0,.type =
        GPIO_ALTF_B,},    
    {.altfun = GPIO_ALT_SD_CARD0,.start = 18,.end = 28,.cont = 0,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_MM_CARD0,.start = 18,.end = 32,.cont = 0,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_USB_OTG,.start = 256,.end = 267,.cont = 0,.type =
        GPIO_ALTF_A,},
    {.altfun = GPIO_ALT_EMMC,.start = 197,.end = 207,.cont = 0,.type =
        GPIO_ALTF_A,},
#ifdef CONFIG_STM_8500_V1
    {.altfun = GPIO_ALT_POP_EMMC,.start = 128,.end = 138,.cont = 0,.type =
        GPIO_ALTF_A,},
#endif
};

/*
 * Static Function declarations
 */
gpio_error gpio_setpinconfig(gpio_pin pin_id, gpio_config * config)
{
    struct gpio_register *p_gpio_register = addr_gpio_register[GPIO_BLOCK(pin_id)];
    u32 mask = 1UL << (pin_id % GPIO_PINS_PER_BLOCK);
    gpio_error gpio_error = GPIO_OK;
    u32 temp_reg;

    switch (config->mode) {
        case GPIO_ALTF_A:
            temp_reg = readl(&p_gpio_register->gpio_afsa);
            temp_reg |= mask;
            writel(temp_reg, &p_gpio_register->gpio_afsa);
            temp_reg = readl(&p_gpio_register->gpio_afsb);
            temp_reg &= ~mask;
            writel(temp_reg, &p_gpio_register->gpio_afsb);
            break;
        case GPIO_ALTF_B:
            temp_reg = readl(&p_gpio_register->gpio_afsa);
            temp_reg &= ~mask;
            writel(temp_reg, &p_gpio_register->gpio_afsa);
            temp_reg = readl(&p_gpio_register->gpio_afsb);
            temp_reg |= mask;
            writel(temp_reg, &p_gpio_register->gpio_afsb);
            break;
        case GPIO_ALTF_C:
            temp_reg = readl(&p_gpio_register->gpio_afsa);
            temp_reg |= mask;
            writel(temp_reg, &p_gpio_register->gpio_afsa);
            temp_reg = readl(&p_gpio_register->gpio_afsb);
            temp_reg |= mask;
            writel(temp_reg, &p_gpio_register->gpio_afsb);
            break;
        case GPIO_MODE_SOFTWARE:
            temp_reg = readl(&p_gpio_register->gpio_afsa);
            temp_reg &= ~mask;
            writel(temp_reg, &p_gpio_register->gpio_afsa);
            temp_reg = readl(&p_gpio_register->gpio_afsb);
            temp_reg &= ~mask;
            writel(temp_reg, &p_gpio_register->gpio_afsb);

            switch (config->direction) {
                case GPIO_DIR_INPUT:
                    writel(mask, &p_gpio_register->gpio_dirc);
                    break;
                case GPIO_DIR_OUTPUT:
                    writel(mask, &p_gpio_register->gpio_dirs);
                    break;
                case GPIO_DIR_LEAVE_UNCHANGED:
                    break;
                default:
                    return (GPIO_INVALID_PARAMETER);
            }

            break;
        case GPIO_MODE_LEAVE_UNCHANGED:
            break;
        default:
            return (GPIO_INVALID_PARAMETER);
    }
    return (gpio_error);
}

gpio_error gpio_resetgpiopin(gpio_pin pin_id, char *dev_name)
{
    struct gpio_register *p_gpio_register = addr_gpio_register[GPIO_BLOCK(pin_id)];
    u32 mask = 1UL << (pin_id % GPIO_PINS_PER_BLOCK);
    gpio_error gpio_error = GPIO_OK;
    u32 temp_reg;

    temp_reg = readl(&p_gpio_register->gpio_afsa);
    temp_reg &= ~mask;
    writel(temp_reg, &p_gpio_register->gpio_afsa);
    temp_reg = readl(&p_gpio_register->gpio_afsb);
    temp_reg &= ~mask;
    writel(temp_reg, &p_gpio_register->gpio_afsb);
    writel(mask, &p_gpio_register->gpio_dirc);

    return (gpio_error);
}

gpio_config altfun_pinconfig;
gpio_error gpio_altfunction(gpio_alt_function alt_func,
        int which_altfunc, char *dev_name)
{
    int i, j, start, end;
    gpio_error error = -1;

    for (i = 0; i < sz_altfun_tbl; i++) {
        if (altfun_table[i].altfun == alt_func)
            break;
    }
    start = altfun_table[i].start;
    end = altfun_table[i].end;
    for (j = start; j <= end; j++) {
        {
            if (which_altfunc == GPIO_ALTF_FIND) {
                altfun_pinconfig.mode =
                    altfun_table[i].type;
            } else {
                altfun_pinconfig.mode = which_altfunc;
            }
            altfun_pinconfig.direction = GPIO_DIR_OUTPUT;
            altfun_pinconfig.dev_name = dev_name;

            if (which_altfunc != GPIO_ALTF_DISABLE) {
                error =
                    gpio_setpinconfig(j,
                            &altfun_pinconfig);
            } else {
                error = gpio_resetgpiopin(j, dev_name);
            }
            if (!error)
                continue;
            nmdk_error
                ("GPIO %d configuration failure (nmdk_error:%d)",
                 j, error);
            error = GPIO_INVALID_PARAMETER;
            return (error);
        }
    }
    return (error);
}

int gpio_writepin(gpio_pin pin_id, gpio_data value, char *dev_name)
{
    struct gpio_register *p_gpio_register = addr_gpio_register[GPIO_BLOCK(pin_id)];
    u32 mask = 1UL << (pin_id % GPIO_PINS_PER_BLOCK);

    switch (value) {
        case GPIO_DATA_HIGH:
            writel(mask, &p_gpio_register->gpio_dats);
            break;
        case GPIO_DATA_LOW:
            writel(mask, &p_gpio_register->gpio_datc);
            break;
        default:
            nmdk_error("Invalid value passed in %s", __FUNCTION__);
            return GPIO_INVALID_PARAMETER;
    }
    return GPIO_OK;
}

int gpio_readpin(gpio_pin pin_id, gpio_data * p_value)
{
    struct gpio_register *p_gpio_register = addr_gpio_register[GPIO_BLOCK(pin_id)];
    u32 mask = 1UL << (pin_id % GPIO_PINS_PER_BLOCK);

    if ((readl(&p_gpio_register->gpio_dat) & mask) != 0) {
        *p_value = GPIO_DATA_HIGH;
    } else {
        *p_value = GPIO_DATA_LOW;
    }
    return GPIO_OK;
}


int gpio_altfuncenable(gpio_alt_function altfunc, char *dev_name)
{
    return (int)gpio_altfunction(altfunc, GPIO_ALTF_FIND, dev_name);
}

int gpio_altfuncdisable(gpio_alt_function altfunc, char *dev_name)
{
    return (int)gpio_altfunction(altfunc, GPIO_ALTF_DISABLE, dev_name);
}

void gpio_init(void)
{  

    	sz_altfun_tbl = sizeof(altfun_table) / sizeof(altfun_table[0]);

	addr_gpio_register[0] =(void *) IO_ADDRESS(CFG_GPIO_0_BASE);
	addr_gpio_register[1] =(void *) IO_ADDRESS(CFG_GPIO_1_BASE);
	addr_gpio_register[2] =(void *) IO_ADDRESS(CFG_GPIO_2_BASE);
	addr_gpio_register[3] =(void *) IO_ADDRESS(CFG_GPIO_3_BASE);
	addr_gpio_register[4] =(void *) IO_ADDRESS(CFG_GPIO_4_BASE);
	addr_gpio_register[5] =(void *) IO_ADDRESS(CFG_GPIO_5_BASE);
	addr_gpio_register[6] =(void *) IO_ADDRESS(CFG_GPIO_6_BASE);
	addr_gpio_register[7] =(void *) IO_ADDRESS(CFG_GPIO_7_BASE);
	addr_gpio_register[8] =(void *) IO_ADDRESS(CFG_GPIO_8_BASE);

    	return;
}