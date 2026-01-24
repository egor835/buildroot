# Buildroot for csky architecture (gx6605s)

## how to build:

1. Get the docker container
```
docker pull maohan001/ubuntu-buildroot
```
2. Get into the docker container and install ncurses
```
docker run -i -t maohan001/ubuntu-buildroot:latest /bin/bash
apt update && apt install ncurses-dev openjdk-8-jdk
```
3. Clone this repo
```
cd ~
git clone https://github.com/egor835/gx6605s_ultimate_linux
mv gx6605s_ultimate_linux buildroot
cd buildroot/
```
4. Configure
   
If you want uart console:
```
make csky_610_gx6605s_4.9_uclibc_br_defconfig
```
or if you prefer hdmi:
```
make csky_610_gx6605sfb_4.9_uclibc_br_defconfig
```
then
```
make menuconfig
```
5. Build
```
make
```
6. Somehow pull output/images/usb.img.xz from container, unpack and *dd* it to usb

## Useful links:
https://dev.to/maple/build-and-modify-linux-system-image-for-c-sky-based-gx6605s-board-3i7j 
https://c-sky.github.io/docs/gx6605s.html

   
## Original readme:

C-SKY Buildroot 开发流程：

 - master 分支, 每周合并一次, 周一为merge window. (周一发出的 pull-request 会
   被master维护者丢弃, 所以请不要在周一发 pull-request)

 - pull-request 必须基于最新 CI pass 的 master commit (自pull-request邮件发出
   时间点，最新master), 原则上 master 不应该出现 fail.

 - pull-request 自身必须提供 CI pass 的证明链接，并且不允许修改 .yml

发布地址:
   https://github.com/c-sky/buildroot/releases
