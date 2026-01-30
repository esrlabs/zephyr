# Copyright 2025 Accenture.

import sys
import time
import socket

target_addr = "192.0.2.1", 4444
RECV_BUF_SIZE = 65535

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(('192.0.2.2', 0))
s.settimeout(1)

def max_length_test():
    length = 1
    while True:
        print(f'testing length {length}')
        input = str.encode('*'*length)
        s.sendto(input, target_addr)
        try:
            output, fromaddr = s.recvfrom(RECV_BUF_SIZE)
        except TimeoutError:
            return length-1
        if output != input:
            return length-1
        length += 1

def max_throughput_test(packet_length):
    print('running througput test')
    start_time = time.time()
    num_packets = 1000
    for i in range(num_packets):
        input = str.encode(f'{i:04d}'+'#'*(packet_length-4))
        s.sendto(input, target_addr)
        output, fromaddr = s.recvfrom(RECV_BUF_SIZE)
        if input != output:
            print(f'failed: sent {input}, received {output}')
    duration = time.time() - start_time
    print(f'{num_packets} packets of size {packet_length} in {duration} seconds')

max_length = max_length_test()
print(f'max length {max_length}')

max_throughput_test(max_length)
