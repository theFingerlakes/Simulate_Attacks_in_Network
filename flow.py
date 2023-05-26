#! /usr/bin/python
from xml.etree import ElementTree as ET
import sys
import matplotlib.pyplot as pylab
et=ET.parse(sys.argv[1])
rxBytesAll = 0
delayAll = 0
rxPacketsAll = 0
lostPacketsAll = 0
for flow in et.findall("FlowStats/Flow"):
    for tpl in et.findall("Ipv4FlowClassifier/Flow"):
        if tpl.get('flowId')==flow.get('flowId'):
            break
        if tpl.get('destinationPort')=='654':
            continue
        loss = int(flow.get('lostPackets'))
        lostPacketsAll += loss
        rxPackets=int(flow.get('rxPackets'))
        rxBytes=int(flow.get("rxBytes"))
        rxPacketsAll += rxPackets
        rxBytesAll += rxBytes
        if rxPackets!=0:
            t0=float(flow.get('timeFirstRxPacket')[:-2])
            t1=float(flow.get("timeLastRxPacket")[:-2])
            duration=(t1-t0)*1e-9
            delaySum = float(flow.get('delaySum')[:-2])
            delayAll += delaySum

print("packet loss rate: {}".format(lostPacketsAll/(lostPacketsAll+rxPacketsAll)))
print("throughput: {}".format(8*rxBytesAll/102400))
print("delay: {}".format(delayAll*1e-9/rxPacketsAll))
