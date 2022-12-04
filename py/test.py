#! /usr/bin/env python3

import pyvisa

rm = pyvisa.ResourceManager()
#reslist = rm.list_resources("USB?::?*::INSTR")
reslist = rm.list_resources()
print(reslist)