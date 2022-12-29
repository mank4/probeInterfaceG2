#!/usr/bin/env python3
%matplotlib qt5

import pyvisa
import time
import sys
import numpy as np
import time
import matplotlib.pyplot as plt

x = np.linspace(1,50,100)
y = np.zeros(100)

rm = pyvisa.ResourceManager()
reslist = rm.list_resources("USB?::5824::?*::INSTR")
print(reslist)

if (len(reslist) == 0):
    sys.exit()
	
inst = rm.open_resource(reslist[0]);
inst.timeout = 3000 

inst.write("SPI:SCK 16")
inst.write("SPI:MOSI 17")
inst.write("SPI:MISO 18")
inst.write("SPI:CS 19")

inst.write("SPI:CPHA 0")
assert(inst.query("SYST:ERR?").__contains__("No error"))

inst.write("SPI:CPOL 0")
assert(inst.query("SYST:ERR?").__contains__("No error"))

baud = 0.5
inst.write("SPI:BAUD " + str(baud))

plt.ion()
fig = plt.figure()
ax = fig.add_subplot(111)
line1, = ax.plot(x,y,'b-')
plt.ylim([-32768, 32767])
#plt.ylim([-2000, 2000])

spiTx = np.array([0x20, 0x0f])
#print(spiTx)
inst.write_binary_values("SPI:TRANS? ", spiTx, datatype='B')
spiRx = inst.read_binary_values('B')

while True:
    spiTx = np.array([0x80 | 0x40 | 0x29, 0x00, 0x00])
    #print(spiTx)
    inst.write_binary_values("SPI:TRANS? ", spiTx, datatype='B')
    spiRx = inst.read_binary_values('B')
    spiRx = (-(spiRx[1] & 0x80) + (spiRx[1] & 0x7f))*256 + spiRx[2]
    #break
    y = np.roll(y,-1)
    y[-1] = spiRx
    #print(y)
    line1.set_ydata(y)
    fig.canvas.draw()
    fig.canvas.flush_events()
    time.sleep(0.05)
    #break

inst.close()

print(spiRx)