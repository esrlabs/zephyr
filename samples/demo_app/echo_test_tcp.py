# Copyright 2025 Accenture.

import sys
import time
import socket
import struct

target_addr = "192.0.2.1", 1234
RECV_BUF_SIZE = 65535

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('192.0.2.2', 0))
s.connect(target_addr)
#s.settimeout(1)
s.setblocking(False)

def echo_test():
    output = b''

    for i in range(1000):
        input = str.encode(f'{i%10}'*50)
        print(f'sending {i} {input}')
        s.send(input)
        time.sleep(0.1)
        try:
            out, fromaddr = s.recvfrom(RECV_BUF_SIZE)
            output += out
            while len(output) >= 4:
                part, output = output[:4], output[4:]
                ack = struct.unpack('>I', part)[0]
                print(f'acknowledged {ack}')
        except BlockingIOError:
            pass

echo_test()
