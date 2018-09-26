==========================
编译
==========================

------------
推荐环境
------------

推荐使用以下操作系统版本：

* ubuntu 16.04 64位
* windows 7 64位
* windows 10 64位

---------------------
获取源码或可执行程序
---------------------

C-SKY QEMU 可执行程序有以下几种获取方式：

1. CDS安装后，以windows默认安装为例，可以在C:\Program Files (x86)\C-Sky\C-Sky Development Suite\qemu下找到。
2. CDK安装后，以windows默认安装为例，可以在C:\Program Files (x86)\C-Sky\C-Sky Development Kit\CSKY\qemu下找到。
3. 从 C-SKY 官方获取其他可执行版本。

C-SKY QEMU 源码有以下几种获取方式：

1. 从 C-SKY 官方获取 C-SKY QEMU 源码。
2. 从 github 等开源网站获取 C-SKY QEMU 源码。

-------------------
从源码编译
-------------------

这节，以从源码编译ABIV2的C-SKY QEMU为例，描述了如何从源码编译C-SKY QEMU。


**编译：**

.. code-block:: none

  mkdir build
  ../configure --target-list=cskyv2-softmmu
  make

如果需要安装可执行程序到本机执行目录，则可以 **安装：**

.. code-block:: none

  make install

