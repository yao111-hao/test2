# onic-driver

**[onic-driver](https://github.com/Hyunok-Kim/onic-driver)** is yet another open-source driver developed by [Hyunok Kim](https://github.com/Hyunok-Kim) for the AMD [OpenNIC shell](https://github.com/Xilinx/open-nic-shell), adapted from AMD [QDMA Ethernet Platform (QEP) driver](https://github.com/Xilinx/qep-drivers) and [OpenNIC driver](https://github.com/Xilinx/open-nic-driver).

Compared to [OpenNIC driver](https://github.com/Xilinx/open-nic-driver), **onic-driver** provides improved support for various QDMA features. This enhanced support is achieved by the [QEP driver](https://github.com/Xilinx/qep-drivers), which is built upon AMD [libqdma](https://github.com/Xilinx/dma_ip_drivers/tree/master/QDMA/linux-kernel/driver/libqdma).

**RecoNIC** (<ins>R</ins>DMA-<ins>e</ins>nabled <ins>C</ins>ompute <ins>O</ins>ffloading on Smart<ins>NIC</ins>) leverages onic-driver as its software networking stack. Moreover, **RecoNIC** extends onic-driver to support data copy between host and device's memory over PCIe.

## How to use

* compile and load the kernel module
```
$ make
$ sudo insmod onic.ko
```

* remove the kernel module
```
$ sudo rmmod onic
```

## How to test data copy feature

Please refer to RecoNIC's README file.