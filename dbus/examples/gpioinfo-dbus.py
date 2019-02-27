#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
#

import pydbus

if __name__ == '__main__':
    bus = pydbus.SystemBus()
    intf = bus.get('org.gpiod1')
    chip_objs = intf.GetManagedObjects()

    for chip_obj in sorted(chip_objs):
        chip = bus.get('org.gpiod1', chip_obj)
        print('{} - {} lines:'.format(chip.Name, chip.NumLines))
        line_objs = chip.GetManagedObjects()

        for line_obj in sorted(line_objs):
            line = bus.get('org.gpiod1', line_obj)
            offset = line.Offset
            name = line.Name
            consumer = line.Consumer
            output = line.Output
            active_low = line.ActiveLow
            used = line.Used
            open_drain = line.OpenDrain
            open_source = line.OpenSource

            flags = '{} {} {}'.format('used' if used else '',
                                      'open-drain' if open_drain else '',
                                      'open-source' if open_source else '')
            flags = flags.strip()
            if flags:
                flags = ' [{}]'.format(flags)

            print('\tline {:>3}: {:>18} {:>12} {:>8} {:>10}{}'.format(
                  offset,
                  'unnamed' if not name else name,
                  'unused' if not consumer else consumer,
                  'output' if output else 'input',
                  'active-low' if active_low else 'active-high',
                  flags if flags else ''))
