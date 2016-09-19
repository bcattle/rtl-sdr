/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * rtl_test, test and benchmark tool
 *
 * Copyright (C) 2016 by Bryan Cattle <bryan.cattle+sdr@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#include "getopt/getopt.h"
#endif

#include "rtl-sdr.h"
#include "convenience/convenience.h"

static rtlsdr_dev_t *dev = NULL;

char* itoa(int val, int base);

void usage(void)
{
 	fprintf(stderr,
 		"rtl_gpio, a tool to read and write the GPIO bits of RTL2832 based DVB-T receivers\n\n"
 		"Usage:\n"
 		"\t[-s bit=val] Configures the specified bit (0..7) as an output and sets its value (0..1)\n\n"
		"\t[-d device_index (default: 0)]\n");
 	exit(1);
}

int parse(char *cmd, int *bit_to_set, int *val_to_set) {
  int res = 0, bit, val;
  res = sscanf(cmd, "%d=%d", &bit, &val);
  if (res != 2) {
    fprintf(stderr, "Invalid command string.\n");
    exit(1);
  }
  *bit_to_set = bit;
  *val_to_set = val;
  return 0;
}

int main(int argc, char **argv)
{
  int bit_to_set = -1, val = 0, opt, r;
	int dev_index = 0, dev_given = 0;
  uint8_t gpio_val;
  char *buffer;

  while ((opt = getopt(argc, argv, "s:h:d")) != -1) {
		switch (opt) {
		case 's':
      // we expect optarg to be of the form "(0..7)=(0..1)"
      parse(optarg, &bit_to_set, &val);
			break;
    case 'd':
      dev_index = verbose_device_search(optarg);
      dev_given = 1;
      break;
    case 'h':
		default:
			usage();
			break;
		}
	}

  if (!dev_given) {
		dev_index = verbose_device_search("0");
	}

	if (dev_index < 0) {
		exit(1);
	}

  r = rtlsdr_open(&dev, (uint32_t)dev_index);
	if (r < 0) {
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
		exit(1);
	}

  if (bit_to_set != -1) {
    if (bit_to_set < 0 || bit_to_set > 7) {
      fprintf(stderr, "Bit to set out of range. Should be 0..7\n");
      exit(1);
    }
    if (val < 0 || val > 1) {
      fprintf(stderr, "Value to set out of range. Should be 0..1\n");
      exit(1);
    }
    fprintf(stderr, "Setting pin %d to 0b%d\n", bit_to_set, val);
    r = rtlsdr_set_gpio_output(dev, bit_to_set);
    if (r < 0) {
      fprintf(stderr, "Error configuring pin %d as output. Returned error %d\n", bit_to_set, r);
      exit(1);
    }
    r = rtlsdr_set_gpio_bit(dev, bit_to_set, val);
    if (r < 0) {
      fprintf(stderr, "Error setting pin %d value\n", bit_to_set);
      exit(1);
    }
  }
  rtlsdr_get_gpio_byte(dev, &gpio_val);
  buffer = itoa(gpio_val, 2);
  fprintf(stderr, "GPIO byte is set to: 0x%x (0b%s)\n", gpio_val, buffer);
}

char* itoa(int val, int base) {
	static char buf[33] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
}
