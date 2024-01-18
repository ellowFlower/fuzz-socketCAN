#!/bin/bash

FUZZER=$1     #fuzzer name (e.g., aflnet) -- this name must match the name of the fuzzer folder inside the Docker container
OUTDIR=$2     #name of the output folder
OPTIONS=$3    #all configured options -- to make it flexible, we only fix some options (e.g., -i, -o, -N) in this script
TIMEOUT=$4    #time for fuzzing
SKIPCOUNT=$5  #used for calculating cov over time. e.g., SKIPCOUNT=5 means we run gcovr after every 5 test cases
NO_SEEDS=$6
strstr() {
  [ "${1#*$2*}" = "$1" ] && return 1
  return 0
}

#Commands for afl-based fuzzers (e.g., aflnet, aflnwe)
if $(strstr $FUZZER "afl"); then
  #Step-1. Do Fuzzing
  #Move to fuzzing folder
  cd $WORKDIR
  if [ "$NO_SEEDS" = 1 ]; then
    INPUTS="$WORKDIR/in-dtls-empty"
  else
    INPUTS="$WORKDIR/in-dtls"
  fi
  if [ "$FUZZER" = "aflpp" ]; then
    AFL_PRELOAD="/home/ubuntu/preeny/src/desock.so" \
      timeout -k 0 $TIMEOUT /home/ubuntu/${FUZZER}/afl-fuzz \
        -d -i "$INPUTS" -o $OUTDIR $OPTIONS \
        ./tinydtls/tests/dtls-server
  else
    timeout -k 0 $TIMEOUT /home/ubuntu/${FUZZER}/afl-fuzz \
      -d -i "$INPUTS" -o $OUTDIR -N udp://127.0.0.1/20220 $OPTIONS \
      ./tinydtls/tests/dtls-server
  fi
  wait 

  #Step-2. Collect code coverage over time
  #Move to gcov folder
  cd $WORKDIR

  #The last argument passed to cov_script should be 0 if the fuzzer is afl/nwe and it should be 1 if the fuzzer is based on aflnet
  #0: the test case is a concatenated message sequence -- there is no message boundary
  #1: the test case is a structured file keeping several request messages
  if [ $FUZZER == "aflnwe" ]; then
    cov_script ${WORKDIR}/${OUTDIR}/ 20220 ${SKIPCOUNT} ${WORKDIR}/${OUTDIR}/cov_over_time.csv 0
  elif [ "$FUZZER" = "aflpp" ]; then
    cov_script ${WORKDIR}/${OUTDIR}/default 20220 ${SKIPCOUNT} ${WORKDIR}/${OUTDIR}/cov_over_time.csv 0
  else
    cov_script ${WORKDIR}/${OUTDIR}/ 20220 ${SKIPCOUNT} ${WORKDIR}/${OUTDIR}/cov_over_time.csv 1
  fi

  gcovr -r $WORKDIR/tinydtls-gcov --html --html-details -o index.html
  mkdir ${WORKDIR}/${OUTDIR}/cov_html/
  cp *.html ${WORKDIR}/${OUTDIR}/cov_html/
  # genhtml -o "${WORKDIR}/${OUTDIR}/cov_html/" --branch-coverage "$WORKDIR/coverage.info"

  #Step-3. Save the result to the ${WORKDIR} folder
  #Tar all results to a file
  cd ${WORKDIR}
  tar -zcvf ${WORKDIR}/${OUTDIR}.tar.gz ${OUTDIR}
fi
