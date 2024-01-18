
#include <stdio.h>
#include <stdint.h>
#include "nyx.h"
#include <string.h>
#include <unistd.h>

/**
 * In summary, this code is a simple program that reads input data from standard input and prints it using
 * the hprintf function. It performs basic checks related to the Nyx fuzzer, such as ensuring that it is running 
 * on a Nyx virtual CPU and validating the number of command-line arguments. 
*/
int main(int argc, char** argv){
  char buf[1024];

  if(!is_nyx_vcpu()){
    printf("Error: NYX vCPU not found!\n");
    return 0;
  }

  if(argc != 1){
    printf("Usage: <hcat>\n");
    return 1;
  }

  ssize_t received = 0;
  while((received = read(0, buf, sizeof(buf)-1))>0) {
    buf[1023] = 0;
    buf[received] = 0;

    hprintf("[hcat] %s", buf);
    memset(buf, 0, 1024);
  }
  return 0;
}