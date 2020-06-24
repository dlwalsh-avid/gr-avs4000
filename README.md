# gr-avs4000
GNU Radio module for the [AVS-4000 Transceiver](http://www.avid-systems.com/avs4000.php)

## Building

This GNU Radio module has been only tested on Ubuntu 18.04LTS with 
GNU Radio 3.7.11.

To build the gr_avs4000 module the following needs to be installed:

```
$ sudo apt-get install gnuradio-dev swig cmake build-essential doxygen xterm pkg-config
```

Building GR-AVS4000
```
    $ cd gr-avs4000
    $ mkdir build
    $ cd build
    $ cmake ../
    $ make;make test
    $ sudo make install
    $ sudo ldconfig
```

