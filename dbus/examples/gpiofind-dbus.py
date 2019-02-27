#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
#

import pydbus
import sys

if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise TypeError('usage: {} <line-name>'.format(sys.argv[0]))

    line_name = sys.argv[1]
    bus = pydbus.SystemBus()
    intf = bus.get('org.gpiod1')
    chip_objs = intf.GetManagedObjects()

    for chip_obj in sorted(chip_objs):
        chip = bus.get('org.gpiod1', chip_obj)
        line_objs = chip.GetManagedObjects()

        for line_obj in sorted(line_objs):
            line = bus.get('org.gpiod1', line_obj)
            if line.Name == line_name:
                print('{} {}'.format(chip.Name, line.Offset))
                sys.exit(0)

    # Line not found if reached.
    sys.exit(1)
