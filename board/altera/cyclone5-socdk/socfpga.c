// SPDX-License-Identifier: GPL-2.0+
/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
 */
#include <common.h>

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_TARGET_SOCFPGA_CYCLONE5_MATRIX)

#include <malloc.h>
#include <fs.h>
#include <env.h>
#include <command.h>
#include <linux/delay.h>
#include <led.h>
#include <asm-generic/gpio.h>

int board_late_init(void)
{
	led_default_state();
	return 0;
}

///////// MACRO & CONSTANTS ///////////////////////////////////////////////////

#define ENV_VAR_BPRESS              "BUTTON_PRESSED" //! Env. var set by check button function
#define ENV_VAR_ETH0                "ethaddr"
#define ENV_VAR_ETH1                "eth1addr"
#define ENV_VAR_RESTORE_DEFAULT     "userrestoredefault"
#define ENV_VAR_BOOT_LOADER         "userbootloader"
#define MAC_ADDR0_STR               "macAddress"
#define MAC_ADDR1_STR               "macAddress1"

#define LED_STATUS            "led_status"
#define LED_GOOD_GREEN        "led_good_green"
#define LED_READY             "led_ready"
#define LED_COM               "led_com"
#define LED_TRIGGER           "led_trigger"

#define MODE_STANDARD              0
#define MODE_DEFAULT_WAIT          10
#define MODE_DEFAULT_CONFIRM       11
#define MODE_LOADER                20
#define MODE_DEFAULT_WAIT_TO_MS    4000
#define MODE_DEFAULT_CONFIRM_TO_MS 1000
#define MODE_LOADER_TO_MS          4000
#define MODE_DEFAULT_LED_TO_MS     250


///////// STRUCTURE DEFINITION ////////////////////////////////////////////////


///////// UTILITY FUNCTIONS ///////////////////////////////////////////////////

static int dla_utils_read_file(const char *ifname, const char *dev_part_str, const char *filename, const int hex_len, char *out)
{
	unsigned long addr, bytes;
	loff_t len_read;
	int fstype = FS_TYPE_EXT;

	//check if the partition is right
	if (fs_set_blk_dev(ifname, dev_part_str, fstype))
	{
		printf("\n[%s] Error - Wrong interface=%s or partion=%s\n", __FUNCTION__, ifname, dev_part_str);
		return -1;
	}
	addr  = (unsigned long)out;
	bytes = hex_len;
	if (fs_read(filename, addr, 0, bytes, &len_read) != 0)
	{
		printf("\n[%s] can't read the file %s\n", __FUNCTION__, filename);
		return -2;
	}
	out[len_read] = '\0';

	return len_read;
}

static int dla_utils_read_file_loadaddr(const char *ifname, const char *dev_part_str, const char *filename, char **out)
{
	const char *addr_str = env_get("loadaddr");
	if (addr_str != NULL)
		*out = (char *) simple_strtoul(addr_str, NULL, 16);
	else
	{
		printf("\n[%s] can't read loadaddr from env\n", __FUNCTION__);
		return -1;
	}

	return dla_utils_read_file(ifname, dev_part_str, filename, 0, *out);
}

static char *dla_utils_getAndSetMacAddress(int n, char *in)
{
	char *p, *m;
	char mac[18+1];
	int i, j, k, r;

	switch (n)
	{
		case 0:  p = strstr(in, MAC_ADDR0_STR);  break;
		case 1:  p = strstr(in, MAC_ADDR1_STR); break;
		default: return NULL;
	}
	if (!p) return NULL;

	p += (n==0) ? strlen(MAC_ADDR0_STR) : strlen(MAC_ADDR1_STR);
	p = strstr(p,"=");
	if (!p) return NULL;

	p+=1;
	m=p;
	for (i=0; i<strlen(p); i++)
	{
		if (m[i]==' ' || m[i]=='\t')
			continue;
		for (j=0,k=i, r=0; k<12;) {
			mac[j] = m[k+r];
			j++;
			r++;
			mac[j] = m[k+r];
			j++;
			r++;
			mac[j] = ':';
			j++;
			k+=2;
			r=0;
		}
		mac[j-1] = '\0';
		env_set((n==0) ? ENV_VAR_ETH0 : ENV_VAR_ETH1, mac); 
		p=m+12;
		return p;
	}

	return NULL;
}

static int dla_utils_led_set_state(const char *label, enum led_state_t state)
{
	struct udevice * led;
	if (led_get_by_label(label, &led) != 0)
	{
		printf("error: led lable %s not found\n", label);
		return -1;
	}
	if (led_set_state(led, state) != 0)
	{
		printf("error: cannot set state of %s\n", label);
		return -1;
	}
	return 0;
}

///////// DLA COMMANDS IMPLEMENTATION /////////////////////////////////////////

/**
 * @brief Read MAC addresses from configuration files
 * Usage: <cmd> <ifname> <part> <network_file> (Ex. dlamac mmc 0:5 /Config/cfg/network.ini)
 */
static int dla_read_mac(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int res;
	char *out, *p;
	if (argc < 5)
	{
		printf("\n[%s] Wrong number of arguments\n", __FUNCTION__);
		return 1;
	}
	unsigned long filesize = simple_strtoul(argv[4], NULL , 16);
	if (filesize<=0 && filesize>500)
	{
		printf("\n[%s] Invalid filesize %lu\n", __FUNCTION__, filesize);
		return 2;
	}

	out = malloc(filesize + 1);
	//printf("\n[%s] Read file %s...\n", __FUNCTION__, argv[3]);
	res = dla_utils_read_file(argv[1], argv[2], argv[3], filesize, out);
	if (res>=0) {
		out[res] = '\0';
		//mac for eth0
		p = dla_utils_getAndSetMacAddress(0, out);
		if (!p) {
			printf("\n[%s]Can't found eth0 MAC address\n", __FUNCTION__);
			p = out;
		}
		//mac for eth1
		p = dla_utils_getAndSetMacAddress(1, out);
		if (!p) {
			//printf("\n[%s]Can't found eth1 MAC address\n", __FUNCTION__);
			p = out;
		}
	} else {
		printf("\n[%s] can't read file=%s error=%d\n", __FUNCTION__, argv[3], res);
	}
	free(out);

	return 0;
}

/**
 * @brief Check if the button is pressed
 * Usage: <cmd>
 */
static int is_button_pressed(void)
{
	struct gpio_desc * gpio;
	if (gpio_hog_lookup_name("xpress_button", &gpio) != 0)
	{
		printf("failed to retrieve xpress_button gpio\n");
		return 0;
	}
	return dm_gpio_get_value(gpio);
}

/**
 * @brief Sample button and set env var ENV_VAR_BPRESS
 * Usage: <cmd>
 */
static int dla_button_check(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	env_set(ENV_VAR_BPRESS, is_button_pressed() ? "true" : "false");
	return 0;
}

/**
 * @brief TODO
 * Usage: <cmd>
 */
static int dla_button_mng(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 button_pressed = 0;
	u32 reset_leds = 0;
	u32 status = MODE_STANDARD;
	u32 t0, t, tLed0, tLed, ledCount = 0;

	if (is_button_pressed() == 0)
	{
		return 0;  // Button is not pressed, continue with normal boot
	}

	// Manage button
	status = MODE_DEFAULT_WAIT;
	t0 = get_timer(0);
	tLed0 = 0;

	do
	{
		udelay(1000);

		button_pressed = is_button_pressed();

		// Manage status
		switch (status)
		{
			case MODE_DEFAULT_WAIT:
				if (button_pressed)
				{
					t = get_timer(t0);
					if (t > MODE_DEFAULT_WAIT_TO_MS)
					{
						// Time elapsed, move to loader selection
						status = MODE_LOADER;
						t0 = get_timer(0);
					}
				}
				else
				{
					// Button released, If it will be pressed again restore default else exit
					status = MODE_DEFAULT_CONFIRM;
					t0 = get_timer(0);
				}
				break;

			case MODE_DEFAULT_CONFIRM:
				if (button_pressed)
				{
					printf("\n[%s] User select <restore defaults>.\n", __FUNCTION__);
					env_set(ENV_VAR_RESTORE_DEFAULT, "true");
					return 0;
				}
				t = get_timer(t0);
				if (t > MODE_DEFAULT_CONFIRM_TO_MS)
				{
					// Button has not been pressed again, exit
					status = MODE_STANDARD;
				}
				break;

			case MODE_LOADER:
				if (button_pressed)
				{
					t = get_timer(t0);
					if (t > MODE_LOADER_TO_MS)
					{
						// Time elapsed, move to restore default selection
						status = MODE_DEFAULT_WAIT;
						reset_leds = 1;
						t0 = get_timer(0);
					}
				}
				else
				{
					// Button released, boot loader image
					printf("\n[%s] User select <loader>.\n", __FUNCTION__);
					env_set(ENV_VAR_BOOT_LOADER, "true"); 
					status = MODE_STANDARD;
				}
				break;

			case MODE_STANDARD:
				break;

			default:
				return 1;
		}

		// Manage leds
		switch (status)
		{
			case MODE_DEFAULT_WAIT:
				if (reset_leds == 1)
				{
					dla_utils_led_set_state(LED_STATUS, LEDST_OFF);
					dla_utils_led_set_state(LED_COM, LEDST_OFF);
					dla_utils_led_set_state(LED_TRIGGER, LEDST_OFF);
					dla_utils_led_set_state(LED_GOOD_GREEN, LEDST_OFF);
					dla_utils_led_set_state(LED_READY, LEDST_OFF);
					reset_leds = 0;
				}
				tLed = get_timer(tLed0);
				if (tLed > MODE_DEFAULT_LED_TO_MS)
				{
					switch (ledCount) {
						case 0:
							dla_utils_led_set_state(LED_STATUS, LEDST_ON);
							dla_utils_led_set_state(LED_COM, LEDST_ON);
							dla_utils_led_set_state(LED_TRIGGER, LEDST_ON);
							dla_utils_led_set_state(LED_GOOD_GREEN, LEDST_ON);
							dla_utils_led_set_state(LED_READY, LEDST_ON);
							break;
						case 1:
							dla_utils_led_set_state(LED_STATUS, LEDST_OFF);
							dla_utils_led_set_state(LED_COM, LEDST_OFF);
							dla_utils_led_set_state(LED_TRIGGER, LEDST_OFF);
							dla_utils_led_set_state(LED_GOOD_GREEN, LEDST_OFF);
							dla_utils_led_set_state(LED_READY, LEDST_OFF);
							break;
					}
					ledCount = !ledCount;
					tLed0 = get_timer(0);
				}
				break;

			case MODE_LOADER:
				dla_utils_led_set_state(LED_STATUS, LEDST_ON);
				dla_utils_led_set_state(LED_COM, LEDST_ON);
				dla_utils_led_set_state(LED_TRIGGER, LEDST_ON);
				dla_utils_led_set_state(LED_GOOD_GREEN, LEDST_ON);
				dla_utils_led_set_state(LED_READY, LEDST_ON);
				break;

			case MODE_DEFAULT_CONFIRM:
			case MODE_STANDARD:
				dla_utils_led_set_state(LED_STATUS, LEDST_OFF);
				dla_utils_led_set_state(LED_COM, LEDST_OFF);
				dla_utils_led_set_state(LED_TRIGGER, LEDST_OFF);
				dla_utils_led_set_state(LED_GOOD_GREEN, LEDST_OFF);
				dla_utils_led_set_state(LED_READY, LEDST_OFF);
				break;

			default:
				return 1;
		}
	}
	while(status != MODE_STANDARD);

	return 0;
}

/**
 * @brief Blink all leds
 * Usage: <cmd> (Ex. dlablinkall)
 */
static int dla_led_blink_all(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long ret = 1;
	enum led_state_t val;
	if (argc != 1)
	{
		printf("\n[%s] Wrong number of arguments\n", __FUNCTION__);
		return 1;
	}

	val = LEDST_ON;

	ret &= dla_utils_led_set_state(LED_STATUS, val);
	ret &= dla_utils_led_set_state(LED_GOOD_GREEN, val);
	ret &= dla_utils_led_set_state(LED_READY, val);
	ret &= dla_utils_led_set_state(LED_COM, val);
	ret &= dla_utils_led_set_state(LED_TRIGGER, val);	

	udelay(300*1000);

	val = LEDST_OFF;

	ret &= dla_utils_led_set_state(LED_STATUS, val);
	ret &= dla_utils_led_set_state(LED_GOOD_GREEN, val);
	ret &= dla_utils_led_set_state(LED_READY, val);
	ret &= dla_utils_led_set_state(LED_COM, val);
	ret &= dla_utils_led_set_state(LED_TRIGGER, val);	

	if (ret != 0)
	{
		printf("\n[%s] failed to set some leds\n", __FUNCTION__);
		return 3;
	}

	return 0;
}

/**
 * @brief Make a beep
 * Usage: <cmd> (Ex. dlabeep)
 */
static int dla_beep(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32* baseReg;
	char* buffer = NULL;
	char filename[128];
	char part[10];
	char* mmcloadp;
	char* boot_path;
	char* token;
	char* baseStr;
	unsigned long baseRegLong;

	if (argc != 1)
	{
		printf("\n[%s] Wrong number of arguments\n", __FUNCTION__);
		return 1;
	}

	mmcloadp = env_get("mmcloadp");
	if (mmcloadp == NULL)
	{
		printf("\n[%s] mmcloadp not found\n", __FUNCTION__);
		return 1;
	}

	sprintf(part, "0:%s", mmcloadp);

	boot_path = env_get("boot_path");
	if (boot_path == NULL)
	{
		printf("\n[%s] boot_path not found\n", __FUNCTION__);
		return 2;
	}

	sprintf(filename, "/%s/Info/version-fpga.ini", boot_path);
	
	// WARNING: we're assuming that the file is not bigger than 1024B
	//          else we don't beep!
	if (dla_utils_read_file_loadaddr("mmc", part, filename, &buffer) <= 0)
	{
		printf("\n[%s] file \"%s\" not found\n", __FUNCTION__, filename);
		return 3;
	}

	token = strstr(buffer, "CVHPS_ARM_A9_0_AIODMMISC_BASE=");
	if (token == NULL)
	{
		printf("\n[%s] CVHPS_ARM_A9_0_AIODMMISC_BASE not found\n", __FUNCTION__);
		return 4;
	}

	baseStr = token + strlen("CVHPS_ARM_A9_0_AIODMMISC_BASE=");

	baseRegLong = simple_strtoul(baseStr, NULL , 16);

	// read the base address from the file (if any)
	baseReg = (u32*)baseRegLong;

	// set up the beeper output peripheral
	*(baseReg + 2) = 0xFFFF0001; // Control: enable the peripheral
	*(baseReg + 3) = 0x00000000; // Output Main: reset all.
	*(baseReg + 4) = 0x00000000; // Output Delay: set to 0 us.
	*(baseReg + 5) = 0x00000143; // Output T1: set to 323 us.
	*(baseReg + 6) = 0x00000143; // Output T0: set to 323 us.

	// make a beep!
	*(baseReg + 3) = 0x58000001; // Output Main: trigger PWM generator.

	// wait one second
	mdelay(1000);

	// switch off beep
	*(baseReg + 3) = 0x51000001; // Output Main: stop PWM generator.

	// Control: disable and reset peripheral
	*(baseReg + 2) = 0xFFFF0002;

	return 0;
}

////////// UBOOT COMMAND DEFINITION ///////////////////////////////////////////

U_BOOT_CMD(
	dlamac, 5, 0, dla_read_mac,
	"Read MAC addresses from config. file",
	"Usage: dlamac <ifname> <part> <network.ini> <filesize>"
)

U_BOOT_CMD(
	dlabuttoncheck, 1, 0, dla_button_check,
	"Check Xpress button", "Usage: dlabuttoncheck (set " ENV_VAR_BPRESS " var)"
)

U_BOOT_CMD(
	dlabuttonmng, 1, 0, dla_button_mng,
	"Manage Xpress button", "Usage: dlabuttonmng"
)

U_BOOT_CMD(
	dlablinkall, 1, 0, dla_led_blink_all,
	"Set the value of all leds",
	"Usage: dlablinkall\n"
)

U_BOOT_CMD(
	dlabeep, 1, 0, dla_beep,
	"Make a beep!",
	"Usage: dlabeep\n"
)

#endif // CONFIG_TARGET_SOCFPGA_CYCLONE5_SOCDK