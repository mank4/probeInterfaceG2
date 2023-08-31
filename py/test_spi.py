#!/usr/bin/env python3

import pyvisa
import time
import sys
import numpy as np
import time

rm = pyvisa.ResourceManager()
#print(rm.list_resources())
#reslist = rm.list_resources("USB?::5824::?*::INSTR")
reslist = rm.list_resources("USB?::0x16C0::?*::INSTR")
print(reslist)

if (len(reslist) == 0):
    sys.exit()
	
inst = rm.open_resource(reslist[0]);
inst.timeout = 3000 

inst.write("IO:LEV 0.8")

inst.write("IO4:FU OUT")
inst.write("IO4:WRITE 1")
inst.write("IO4:EN 1")
#time.sleep(0.1)
inst.write("IO4:WRITE 0")
time.sleep(0.1)
inst.write("IO4:WRITE 1")

#inst.close()

inst.write("IO1:FUnc CS")
inst.write("IO3:FUnc SCK")
inst.write("IO5:FUnc MOSI")
inst.write("IO7:FUnc MISO")

inst.write("IO1:EN 1")
inst.write("IO3:EN 1")
inst.write("IO5:EN 1")
inst.write("IO7:EN 1")

inst.write("SPI:CPHA 0")
assert(inst.query("SYST:ERR?").__contains__("No error"))

inst.write("SPI:CPOL 0")
assert(inst.query("SYST:ERR?").__contains__("No error"))

baud = 7
inst.write("SPI:BAUD " + str(baud))
baudRead = float(inst.query("SPI:BAUD?"))
assert(baud+0.01 > baudRead)
assert(baud-0.01 < baudRead)

transferLen = 1000
spiTx = np.random.randint(0,255,transferLen)
#print(spiTx)
watch = time.perf_counter()
inst.write_binary_values("SPI:TRANS? ", spiTx, datatype='B')
spiRx = inst.read_binary_values('B')
watch = time.perf_counter() - watch
spiRx = np.asarray(spiRx)
#print(spiRx)

inst.write("*RST")

inst.close()

assert(np.array_equal(spiTx, spiRx))
print(f"SPI tranfer took: {watch*1000:0.4f} ms")
print(f"This is {transferLen/1000/watch:0.4f} kbyte/s")
