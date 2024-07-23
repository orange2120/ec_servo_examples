# EtherCAT Servo Motor Examples

EtherCAT servo motor control examples based on NXP `real-time-edge-servo` (`libnservo`) library.

## Device configuration file

Replace the `VendorId` and the `ProductCode` with your devices.

```xml
...
<VendorId>#x66668888</VendorId>
<ProductCode>#x20193303</ProductCode>
<Name>IHSS57/60-EC</Name>
...
```

## Build

Configure and build
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```
The binaries will be in `./build/bin`

Compile specific target
```bash
### Make Profile Position control example ###
$ make ec_pp_udp_demo
### Make Profile Velocity control example ###
$ make ec_pv_udp_demo
```

## Usage

The example programs in `./example` listen commands from UDP client, the default port is 3000.

Run
```bash
### Run Profile Position control server ###
$ ./ec_pp_udp_demo <motor configuration xml>
### Run Profile Velocity control server ###
$ ./ec_pv_udp_demo <motor configuration xml>
```

Testing using `nc`
```bash
$ nc -u <IP address> <port>
$ nc -u 192.168.50.100 3000
```

The Python script provides simple GUI for examples
```bash
$ python rt_edge_servo_udp_controller.py
```


### ec_pp_udp_demo

Modified from `Profile_Position_Mode_Test_2HSS458-EC.c`, operate the motor in  Profile Position mode.

Message format
```
P<absolute position>,V<profile velocity>
P12000,V500
P-1000,V10000
```

### ec_pv_udp_demo

Modified from `Profile_Velocty_Mode_Test_2HSS458-EC.c`, operate the motor in Profile Velocity mode.

Message format
```
V<target velocity>
V1000
V-30000
```

## Dependencies

```
C compiler
CMake
IgH EtherCAT Master (EtherLab::ethercat)
libxml2-dev
```

## Reference

- [NXP Real-Time Edge Software](https://www.nxp.com/design/design-center/software/development-software/real-time-edge-software:REALTIME-EDGE-SOFTWARE)
- [real-time-edge-servo](https://github.com/nxp-real-time-edge-sw/real-time-edge-servo)