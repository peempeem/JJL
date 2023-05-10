from usbcom import USBMessageBroker
from rate import Rate
import struct

if __name__ == '__main__':    
    broker = USBMessageBroker("COM7", baudrate=500000)
    send = Rate(10)
    

    while True:
        broker.update()
        if send.ready():
            broker.send(2, struct.pack('>B', 100))

    