The folder `vuln-progr` contains a small  program with a buffer overflow vulnerability. I want to fuzz it. The `vuln-progr-specs` folder represents the nyx-net specs folder for it.



For setup follow https://github.com/RUB-SysSec/nyx-net#setup. Rust version has to be `1.53.0` and kvm vm backdor has to be set:

```
sudo modprobe -r kvm-intel
sudo modprobe -r kvm
sudo modprobe  kvm enable_vmware_backdoor=y
sudo modprobe  kvm-intel

curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup install 1.53.0
rustup override set 1.53.0
```

To instal qemu follow this tutorial: https://www.tecmint.com/install-qemu-kvm-ubuntu-create-virtual-machines/.

In `targets/packed_targets` examples provided from nyx-net can be found. They are already in the correct format.

To understand how to use nyx-net I looked at the dnsmasq example, because it seems one of the trivial examples.

The steps to fuzz something are:

1. Obtain the target binary.
2. Pick or create a protocol specification and build the binary.
3. Obtain seed inputs (optionally). => I did not do it.
4. Bundle all required data.
5. Run the fuzzer.

**Pick or create a protocol specification and build the binary**

A protocol specification (spec) is based on the nyx protocol specification. The difference is that it can be created through python methods provided by nyx-net. This is done because in some more advanced protocols the specification is cumbersome to do otherwise. It helps the fuzzer to create input for the target. The provided specs can be found in `targets/specs`. `build` contains a custom C header file, which seems to be the same always. `pcaps` and `raw_streams` are containing target input data in pcap and binary format. This can be provided manually via tcdump when running the target.  `send_code_include.c` seems to be the same always. The most interesting file is `nyx_net_spec.py`, because it has to be written by us and contains the protocol specification used to fuzz the target. I added comments to the provided `dnsmasq` one to understand it better:

```python
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

with open("send_code.include.c") as f:
    send_code = f.read() 


"""
    a Spec contains multiple nodes; these are created in this block;
"""
# d_byte will be a IntDataType, 1 byte long
d_byte = s.data_u8("u8", generators=[limits(0x0, 0xff)])

# d_bytes will be VecDataType; the name "pkt_content" suggests this is
# the content of the packet (packet is the input to the target)
d_bytes = s.data_vec("pkt_content", d_byte, size_range=(0,1<<12), generators=[]) #regex(pkt)

# s.node_type creates a Spec node and appends it to the current Spec;
# the name "packet" suggests this is the whole packet sent;
n_pkt = s.node_type("packet", interact=True, data=d_bytes, code=send_code)

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


"""
    build the current Spec;
"""
s.build_interpreter()


"""
    serialize the builded spec and write it to a file;
    the file will be used from Nyx-Net as input;
"""
import msgpack
serialized_spec = s.build_msgpack()
with open("nyx_net_spec.msgp","wb") as f:
    f.write(msgpack.packb(serialized_spec))

# not used
def split_packets(data):   
    i = 0
    res = []
    while i*6 < len(data):
        tt,content_len = struct.unpack(">2sI",data[i:i+6])  
        res.append( ["dicom", data[i:i+content_len]] )
        i+=(content_len+6)
    return res

def stream_to_bin(path,stream):
    b.packet(stream)
    b.write_to_file(path+".bin")

import pyshark
import glob
import ipdb

"""
# convert existing pcaps
for path in glob.glob("pcaps/*.pcap"):
    b = Builder(s)
    cap = pyshark.FileCapture(path, display_filter="udp.dstport eq 53", include_raw=True, use_json=True)

    #ipdb.set_trace()
    stream = b""
    for pkt in cap:
        #ipdb.set_trace()
        if int(pkt.udp.length) > 0:
            data = bytearray.fromhex(pkt.dns_raw.value)
            print("LEN: ", repr((pkt.udp.length, data)))
            b.packet(data)
    b.write_to_file(path+".bin")
    cap.close()
"""


"""
    current Spec is in AFLNet sample format;
    convert it to nyx-net format and write to file;
    file again used as input from Nyx-Net later;
    should be similiar for all targets;
    the used .raw file represents sampel input for the target captured by tcpdump (has to be done manually);
"""
for path in glob.glob("raw_streams/*.raw"):
    # s = the created spec
    # create builder object with the created spec
    b = Builder(s)
    with open(path,mode='rb') as f:
        # write spec to file
        stream_to_bin(path, f.read())

```

The target binary should be built with the following flags `make CC=afl-clang-fast CXX=afl-clang-fast++ LD=afl-clang-fast`. 

**Bundle all required data.**

Then it has to be packed in nyx format:

```bash
python ./nyx_packer.py \
     <path to compiled target> \
     <full path for targets/packed_targets/<target> \
     m64 \
     -spec < full path to targets/specs/<target> \
     --nyx_net
```

Then a configuration has to be created:

```bash
python ./nyx_config_gen.py <full path to targets/packed_targets/<target> Kernel
```

To make it work in the output file `targets/packed_targets/<target>/config.ron` the value `include_default_config_path` has to be changed to `../default_config_vm.ron`.

More on the packing and config part can be found here https://github.com/AFLplusplus/AFLplusplus/blob/stable/nyx_mode/README.md#pack-libxml2-into-nyx-sharedir-format.

**Run the fuzzer.**

Then we can run the fuzzer. In `fuzzer/rust_fuzzer` run:

```
cargo run --release -- \
      -s ../../targets/packed_targets/<target>/
```



When doing it with my own binary the fuzzer freezes with the following output:

```bash
    Finished release [optimized] target(s) in 0.05s
     Running `target/release/rust_fuzzer -s ../../targets/packed_targets/vuln-progr/`
[!] fuzzer: spawning qemu instance #0
[!] libnyx: spawning qemu with:
 /home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64 -kernel /home/martin/Documents/uni/Thesis/nyx-net/targets/packed_targets/bzImage-linux-4.15-rc7 -initrd /home/martin/Documents/uni/Thesis/nyx-net/targets/packed_targets/init.cpio.gz -append nokaslr oops=panic nopti ignore_rlimit_data -display none -serial none -enable-kvm -net none -k de -m 512 -chardev socket,server,path=/tmp/workdir/interface_0,id=kafl_interface -device kafl,chardev=kafl_interface,bitmap_size=69632,worker_id=0,workdir=/tmp/workdir,sharedir=/home/martin/Documents/uni/Thesis/nyx-net/targets/packed_targets/vuln-progr -machine kAFL64-v1 -cpu kAFL64-Hypervisor-v1,+vmx
[!] Could not access KVM-PT kernel module!
 [*] Trying vanilla KVM...
NYX runs in fallback mode (no Intel-PT tracing or nested hypercall support)!
[*] Max Dirty Ring Size -> 1048576 (Entries: 65536)
Warning: Attempt to use unsupported CPU model (PT) without KVM-PT (Hint: use '-cpu kAFL64-Hypervisor-v2' instead)
qemu-system-x86_64: warning: host doesn't support requested feature: CPUID.07H:EBX.hle [bit 4]
qemu-system-x86_64: warning: host doesn't support requested feature: CPUID.07H:EBX.rtm [bit 11]
[*] Dirty ring mmap region located at 0x7fdc5c282000
[hget] 17200 bytes received from hypervisor! (hcat_no_pt)
[hget] 17168 bytes received from hypervisor! (habort_no_pt)
[hget] 127152 bytes received from hypervisor! (ld_preload_fuzz_no_pt.so)
[hcat] Let's get our dependencies...
[hget] 191504 bytes received from hypervisor! (ld-linux-x86-64.so.2)
[hget] 2029592 bytes received from hypervisor! (libc.so.6)
[hcat] Let's get our target executable...
[hget] 16840 bytes received from hypervisor! (vuln-progr)
[hcat] [    0.787546] target_executab[143]: segfault at 6969464 ip 00007ffff7d414c3 sp 00007fffffffe970 error 4 in libc.so.6[7ffff7c03000+178000]
handle_hypercall_kafl_user_abort: USER_ABORT called!
[qemu-nyx] crash detected (pid: 17429 / signal: 6)
backtrace() returned 12 addresses
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(qemu_backtrace+0x34) [0x55e94d27e2c4]
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(+0x4bb3c0) [0x55e94d27e3c0]
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(+0x4bb40f) [0x55e94d27e40f]
/lib/x86_64-linux-gnu/libpthread.so.0(+0x14420) [0x7fdc5e7b2420]
/lib/x86_64-linux-gnu/libc.so.6(gsignal+0xcb) [0x7fdc5e5ef00b]
/lib/x86_64-linux-gnu/libc.so.6(abort+0x12b) [0x7fdc5e5ce859]
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(handle_kafl_hypercall+0xabe) [0x55e94d274a1e]
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(kvm_cpu_exec+0x3d9) [0x55e94d1c7e89]
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(+0x3dd2dc) [0x55e94d1a02dc]
/home/martin/Documents/uni/Thesis/nyx-net/qemu-nyx/x86_64-softmmu/qemu-system-x86_64(+0x8c18ed) [0x55e94d6848ed]
/lib/x86_64-linux-gnu/libpthread.so.0(+0x8609) [0x7fdc5e7a6609]
/lib/x86_64-linux-gnu/libc.so.6(clone+0x43) [0x7fdc5e6cb133]
WAITING FOR GDB ATTACH (PID: 17429...
```



A reason could be that the target is taking command line input. Maybe nyx-net cannot handle that. 
