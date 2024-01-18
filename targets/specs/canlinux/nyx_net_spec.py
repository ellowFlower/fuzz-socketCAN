import sys, os 
sys.path.insert(1, os.getenv("NYX_INTERPRETER_BUILD_PATH"))

from spec_lib.graph_spec import *
from spec_lib.data_spec import *
from spec_lib.graph_builder import *
from spec_lib.generators import opts,flags,limits,regex

import jinja2

"""
    This file defines the specification used to create inputs for the fuzzer;
    a specification is represented as a direct azyclic graph; 
"""

"""
    class Spec is the input specification;
    in this first block of code general initialization is done, should be same for same for all targets;
"""
s = Spec()
s.use_std_lib = False
s.includes.append("\"custom_includes.h\"")
s.includes.append("\"nyx.h\"")
s.interpreter_user_data_type = "socket_state_t*"

# send_code.include.c: Specifies length of user data and copies
# the data packet content to a buffer called vm->user_data->data
#with open("send_code.include.c") as f:
#    send_code = f.read() 

with open("send_code_raw.include.c") as f:
    send_code_raw = f.read() 


"""
    a Spec contains of multiple nodes; these are created in this block;
    In this example we have two nodes: packet and create_tmp_snapshot;
"""

# d_byte will be a unsigned 8-bit integer with limits 0x0 and 0xff (255)
# 
# limits: creates a LimitsGenerator => not sure what the limits do, could be issue
# u8 often stands for 8-bit unsigned integer type
d_byte = s.data_u8("u8", generators=[limits(0x0, 0xff)])

# d_bytes will be VecDataType; the name "pkt_content" suggests this is
# the content of the packet (packet is the input to the target)
# 1<<12 = 4096
d_bytes = s.data_vec("pkt_content", d_byte, size_range=(0,1<<12), generators=[]) #regex(pkt)

# s.node_type creates a Spec node and appends it to the current Spec;
# the name "packet" suggests this is the whole packet sent;
# n_pkt = s.node_type("packet", interact=True, data=d_bytes, code=send_code)

n_pkt = s.node_type("packet_raw", interact=True, data=d_bytes, code=send_code_raw)

# code used for snapshots is saved in a node and appended to the current Spec;
# should be same for all targets;
snapshot_code="""
//hprintf("ASKING TO CREATE SNAPSHOT\\n");
kAFL_hypercall(HYPERCALL_KAFL_CREATE_TMP_SNAPSHOT, 0);
kAFL_hypercall(HYPERCALL_KAFL_USER_FAST_ACQUIRE, 0);
//hprintf("RETURNING FROM SNAPSHOT\\n");
vm->ops_i -= OP_CREATE_TMP_SNAPSHOT_SIZE;
"""
n_close = s.node_type("create_tmp_snapshot", code=snapshot_code)


# build the current Spec;
s.build_interpreter()

"""
    serialize the builded spec and write it to a file;
    the file will be used from Nyx-Net as input;
"""
import msgpack
serialized_spec = s.build_msgpack()
with open("nyx_net_spec.msgp","wb") as f:
    f.write(msgpack.packb(serialized_spec))

def stream_to_bin(path,stream):
    b.packet(stream)
    b.write_to_file(path+".bin")


import pyshark
import glob
import ipdb

"""
    current Spec is in AFLNet sample format;
    convert it to nyx-net format and write to file;
    file again used as input from Nyx-Net later;
    should be similiar for all targets;
    the used .raw file represents sampel input for the target captured by tcpdump (has to be done manually);
"""
"""
for path in glob.glob("pcaps/*.pcap"):
    b = Builder(s)
    cap = pyshark.FileCapture(path, display_filter="can", include_raw=True, use_json=True)

    for pkt in cap:
        data = bytearray(pkt.data.data.replace(":", "").encode())
        b.packet_raw(data)
    
    b.write_to_file(path+".bin")
    cap.close()
"""
for path in glob.glob("pcaps/*.pcap"):
    b = Builder(s)
    cap = pyshark.FileCapture(path, display_filter="can", include_raw=True, use_json=True)

    stream = b""
    for pkt in cap:
        if int(pkt.data.len) != 0:
            data = bytearray.fromhex(pkt.data.data_raw[0])
            print("DATA: ", repr(data))
            b.packet_raw(data)
    b.write_to_file(path+".bin")
    cap.close()
