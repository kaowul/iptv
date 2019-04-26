#!/bin/bash

USER=iptv_cloud

apt-get update
apt-get install git python3-pip mongodb

git clone https://github.com/fastogt/iptv
cd iptv
git submodule update --init --recursive
cd build

git clone https://github.com/fastogt/pyfastogt
cd pyfastogt
python3 setup.py install
cd ../
rm -rf pyfastogt
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig ./build_env.py

LICENSE_KEY=$(license_gen)
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig ./build.py release $LICENSE_KEY
useradd -m -U -d /home/$USER $USER -s /bin/bash
cd ../
