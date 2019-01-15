FROM debian:stretch-slim

# add our user and group first to make sure their IDs get assigned consistently, regardless of whatever dependencies get added
ENV USER iptv
ENV APP_NAME streamer_service
RUN groupadd -r $USER && useradd -r -g $USER $USER

# grab gosu for easy step-down from root
# https://github.com/tianon/gosu/releases
ENV GOSU_VERSION 1.10
RUN set -ex; \
  FETCH_DEPS='ca-certificates dirmngr gnupg wget'; \
  apt-get update; \
  apt-get install -y --no-install-recommends $FETCH_DEPS; \
  rm -rf /var/lib/apt/lists/*; \
  \
  dpkgArch="$(dpkg --print-architecture | awk -F- '{ print $NF }')"; \
  wget -O /usr/local/bin/gosu "https://github.com/tianon/gosu/releases/download/$GOSU_VERSION/gosu-$dpkgArch"; \
  wget -O /usr/local/bin/gosu.asc "https://github.com/tianon/gosu/releases/download/$GOSU_VERSION/gosu-$dpkgArch.asc"; \
  export GNUPGHOME="$(mktemp -d)"; \
  gpg --batch --keyserver ha.pool.sks-keyservers.net --recv-keys B42F6819007F00F88E364FD4036A9C25BF357DD4; \
  gpg --batch --verify /usr/local/bin/gosu.asc /usr/local/bin/gosu; \
  gpgconf --kill all; \
  rm -r "$GNUPGHOME" /usr/local/bin/gosu.asc; \
  chmod +x /usr/local/bin/gosu; \
  gosu nobody true; \
apt-get purge -y --auto-remove $FETCH_DEPS

RUN set -ex; \
  BUILD_DEPS='git python3 python3-pip'; \
  PREFIX=/usr/local; \
  apt-get update; \
  apt-get install -y ${BUILD_DEPS} --no-install-recommends; \
  # rm -rf /var/lib/apt/lists/*; \
  \
  pip3 install setuptools; \
  PYFASTOGT_DIR=/usr/src/pyfastogt; \
  mkdir -p ${PYFASTOGT_DIR} && git clone https://github.com/fastogt/pyfastogt ${PYFASTOGT_DIR} && cd ${PYFASTOGT_DIR} && python3 setup.py install; \
  \
  IPTV_DIR=/usr/src/iptv; \
  mkdir -p ${IPTV_DIR} && git clone https://github.com/fastogt/iptv ${IPTV_DIR} && cd ${IPTV_DIR} && git submodule update --init --recursive; \
  cd ${IPTV_DIR}/build && PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig python3 build_env.py --prefix_path=$PREFIX; \
  LICENSE_KEY="$(license_gen --machine-id)"; \
  cd ${IPTV_DIR}/build && PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig python3 build.py release $LICENSE_KEY 1 $PREFIX; \
rm -rf $PYFASTOGT_DIR $IPTV_DIR && apt-get purge -y --auto-remove $BUILD_DEPS


RUN mkdir /var/run/$APP_NAME && chown $USER:$USER /var/run/$APP_NAME
VOLUME /var/run/$APP_NAME
WORKDIR /var/run/$APP_NAME

COPY docker-entrypoint.sh /usr/local/bin/
ENTRYPOINT ["docker-entrypoint.sh"]

EXPOSE 6379
CMD [$APP_NAME]
