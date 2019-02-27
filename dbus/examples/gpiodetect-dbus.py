#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2018-2019 Bartosz Golaszewski <bartekgola@gmail.com>
#

import pydbus

if __name__ == '__main__':
    bus = pydbus.SystemBus()
    intf = bus.get('org.gpiod1')
    objs = intf.GetManagedObjects()

    for obj in sorted(objs):
        chip = bus.get('org.gpiod1', obj)
        print('{} [{}] ({} lines)'.format(chip.Name,
                                          chip.Label,
                                          chip.NumLines))
