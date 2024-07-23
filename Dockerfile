FROM ubuntu:20.04
COPY ./tools/requirements.txt /r/
ENV PATH=/opt/cmake/bin:$PATH
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y build-essential cmake git curl patchelf libpam0g-dev software-properties-common libgl1-mesa-dev fakeroot python3-pip zip unzip libnl-genl-3-dev pkg-config libcap-ng-dev wget autoconf libtool libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev
RUN python3 -m pip install -r /r/requirements.txt

RUN wget https://go.dev/dl/go1.22.5.linux-amd64.tar.gz && rm -rf /usr/local/go && tar -C /usr/local -xzf go1.22.5.linux-amd64.tar.gz
RUN ln -sf /usr/local/go/bin/go /usr/local/bin/go

RUN wget https://cmake.org/files/v3.29/cmake-3.29.6-linux-x86_64.sh && mkdir -p /opt/cmake && yes | sh cmake-3.29.6-linux-x86_64.sh --prefix=/opt/cmake
RUN ln -sf /opt/cmake/cmake-3.29.6-linux-x86_64/bin/cmake /usr/bin/cmake

WORKDIR /w
