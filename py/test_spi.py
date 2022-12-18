#!/usr/bin/env python3

import pyvisa
import time
import sys
import numpy as np
import time

rm = pyvisa.ResourceManager()
reslist = rm.list_resources("USB?::5824::?*::INSTR")
print(reslist)

if (len(reslist) == 0):
    sys.exit()
	
inst = rm.open_resource(reslist[0]);
inst.timeout = 3000 

inst.write("SPI:SCK 19")
inst.write("SPI:MISO 18")
inst.write("SPI:MOSI 18")

transferLen = 8000
spiTx = np.random.randint(0,255,transferLen)
#print(spiTx)
watch = time.perf_counter()
inst.write_binary_values("SPI:TRANS? ", spiTx, datatype='B')
spiRx = inst.read_binary_values('B')
watch = time.perf_counter() - watch
spiRx = np.asarray(spiRx)
#print(spiRx)

inst.close()

assert(np.array_equal(spiTx, spiRx))
print(f"SPI tranfer took: {watch*1000:0.4f} ms")
print(f"This is {transferLen/1000/watch:0.4f} kbyte/s")