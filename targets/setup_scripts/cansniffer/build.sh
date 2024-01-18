WORKDIR="$PWD/../build/cansniffer"

ROOT=$PWD
CC=$ROOT/../../../packer/packer/compiler/afl-clang-fast
CPP=$ROOT/../../../packer/packer/compiler/afl-clang-fast++

error () {
  echo "$0: <asan/no_asan>"
  exit 0
}

if test "$#" -ne 1; then
  error
fi

if [ "$1" != "asan" ] && [ "$1" != "no_asan" ] ; then
  error
fi

if test -f "$CC" && test -f "$CPP"; then

  if [ "$1" = "asan" ]; then
    ASAN="-fsanitize=address"
  else 
    ASAN=""
  fi

  rm -rf $WORKDIR

  mkdir $WORKDIR
  cd $WORKDIR && \
  cp -r ../../../../additional-files/cansniffer . && \
  cd cansniffer && \
  CC="$CC $ASAN" make && \
  echo "SUCCESS"

else
  echo "Error: AFL clang compiler not found:\n\t$CC\n\t$CPP"
fi

# /tmp/dnsmasq_build/dnsmasq/src/dnsmasq