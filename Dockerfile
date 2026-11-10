FROM ubuntu:22.04
COPY ./tools/requirements.txt /r/
ENV PATH=/opt/cmake/bin:$PATH
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y build-essential cmake git curl patchelf libpam0g-dev software-properties-common libgl1-mesa-dev fakeroot python3-pip zip unzip libnl-genl-3-dev pkg-config libcap-ng-dev libacl1-dev libnftables-dev wget autoconf libtool libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev
RUN python3 -m pip install -r /r/requirements.txt

RUN wget https://cmake.org/files/v3.29/cmake-3.29.6-linux-x86_64.sh \
    && echo "6e4fada5cba3472ae503a11232b6580786802f0879cead2741672bf65d97488a  cmake-3.29.6-linux-x86_64.sh" | sha256sum -c - \
    && mkdir -p /opt/cmake && yes | sh cmake-3.29.6-linux-x86_64.sh --prefix=/opt/cmake \
    && rm cmake-3.29.6-linux-x86_64.sh
RUN ln -sf /opt/cmake/cmake-3.29.6-linux-x86_64/bin/cmake /usr/bin/cmake

WORKDIR /w
