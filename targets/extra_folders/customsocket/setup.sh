echo "Executing customsocket Setup Script" | ./hcat
ip link set can0 type can bitrate 1000000
ip link set can0 up
ifconfig | grep can0 | ./hcat
echo "customsocket Setup Script finished" | ./hcat 