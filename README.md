Extension of the fuzzer [Nyx-Net](https://github.com/RUB-SysSec/nyx-net) with additional functionality to fuzz SocketCAN targets.

Implemented withing the following [master thesis](thesis.pdf).

## Setup on Ubuntu 22.04

Please note that the minimum requirements to get Nyx-Net running are a recent linux kernel installed (>= v5.11) and full access to KVM. Fast-Snapshots and compile-time based tracing is supported by an unmodified vanilla kernel. If you want to fuzz closed-source targets with Nyx-Net's intel-PT mode, you will need to install [KVM-Nyx](https://github.com/nyx-fuzz/kvm-nyx).

Basic packages have to be installed:
```
sudo apt-get install pkg-config -y && \
sudo apt install build-essential -y && \
sudo apt install curl -y && \
sudo apt-get install libgtk-3-dev -y
```

Rust version has to be `1.53.0` and kvm vm backdor has to be set:

```
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh && \
sudo snap install rustup --classic
rustup install 1.53.0 && \
rustup override set 1.53.0

sudo modprobe -r kvm-intel && \
sudo modprobe -r kvm && \
sudo modprobe  kvm enable_vmware_backdoor=y && \
sudo modprobe  kvm-intel && \
cat /sys/module/kvm/parameters/enable_vmware_backdoor
```

Executing setup.sh will install all further dependencies and setup Nyx-Net an your machine:

Regarding the error `'transmute' is only allowed in constants and statics for now` see additional-files/changes-not-in-commit/time.rs.diff

The setup has been tested with version 11. Use following commands to use llvm and clang version 11:
```
sudo wget --no-check-certificate -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add - && \
sudo add-apt-repository 'deb http://apt.llvm.org/bionic/   llvm-toolchain-bionic-11  main' && \
sudo apt-get install llvm-11 lldb-11 llvm-11-dev libllvm11 llvm-11-runtime -y && \
sudo update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-11 11 && \
sudo update-alternatives --config llvm-config && \
sudo apt install clang-11 lldb-11 lld-11 -y && \
sudo update-alternatives --install /usr/bin/cc cc /usr/bin/clang-11 100 && \
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-11 100 && \
sudo ln -s /usr/bin/clang-11 /usr/bin/clang && \
sudo ln -s /usr/bin/clang++-11 /usr/bin/clang++
```

## Run the fuzzer
Start a CAN interface on the host:
```
sudo ip link add dev can0 type vcan
sudo ip link set up can0
```

Execute files beginning with `all_` to fuzz specific targets. To run the fuzzer in debug mode execute files ending in `_debug.sh`. 

## License

The content of this repository is provided under **AGPL license**. 
However, this does only apply to this repository without any submodule. Please refer to each submodule from this repository to get more detailed information on each license used.
