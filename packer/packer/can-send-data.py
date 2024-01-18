import can
import subprocess
import sys

'''
Run before:
    - fuzzer: ./all_canlinux.sh
    - fuzzer in debug mode: ./canreceive_rust_fuzzer_debug.sh
    - run target locally listening on virtual CAN interface with name can0

Usage: python can-send-data.py <path to crash input> <mode>
mode: can => classic can frame; canfd => canfd frame
E.g.: python can-send-data.py /tmp/workdir/corpus/crash_reproducible/cnt_1.py canfd
'''

def exec_file(path):
    exec(open(path).read())


def get_can_id(can_msg):
    # Split the can_msg into individual bytes
    bytes = can_msg.split('\\x')
    # remove empty elements in list
    bytes = [el for el in bytes if el != '']
    # get can_id part
    can_id_part = bytes[:4]
    can_id = 0
    for byte in reversed(can_id_part):
        can_id <<= 8
        can_id |= int(byte, 16)
    return can_id

def get_can_data(can_msg, data_length):
    # Split the ca_msg into individual bytes
    bytes = can_msg.split('\\x')
    # remove empty elements in list
    bytes = [el for el in bytes if el != '']
    # remove newline
    bytes = [el.replace('\n', '') for el in bytes]
    # get data bytes
    data_part = bytes[data_length:]
    # transform elements of data_part to hex
    data_part = [int(byte, 16) for byte in data_part]
    #TODO maybe reverse
    return data_part

def send_can_message(interface, can_id, data):
    with can.interface.Bus(channel=interface, bustype='socketcan', fd=True) as bus:
        message = can.Message(arbitration_id=can_id, data=data, is_extended_id=False, is_fd=True)
        bus.send(message)
        print("Message: [{}] sent on [{}]".format(message, bus.channel_info))


# parse second argument
if len(sys.argv) != 3:
    print('Usage: python can-send-data.py <path to crash input> <mode>')
    print('mode: can => classic can frame; canfd => canfd frame')
    print('E.g.: python can-send-data.py /tmp/workdir/corpus/crash_reproducible/cnt_1.py')
    sys.exit(1)

content = ''
data = ''
with open(sys.argv[1], 'r') as file:
    content = file.read()
    data = content.replace('packet(data="', '')
    data = data.replace('")', '')
# ASSUMPTION only last target input is the one causing a crash
# last line is empty thus second last line is the last line with data => [-2]
# crash_input = data.split('\n')[-2]
counter = 1
for input_line in data.split('\n'):
    crash_input = input_line
    can_id = get_can_id(crash_input)

    if sys.argv[2] == 'can':
        data = get_can_data(crash_input, -8)
    elif sys.argv[2] == 'canfd':
        data = get_can_data(crash_input, -64)
    else:
        print('Usage: python can-send-data.py <path to crash input> <mode>')
        print('mode: can => classic can frame; canfd => canfd frame')
        print('E.g.: python can-send-data.py /tmp/workdir/corpus/crash_reproducible/cnt_1.py')
        sys.exit(1)

    #print('can_id: {}'.format(can_id))
    #print('data: {}'.format(data))
    send_can_message('can0', can_id, data)


