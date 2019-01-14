#!/bin/sh
set -e

# first arg is `-f` or `--some-option`
# or first arg is `something.conf`
if [ "${1#-}" != "$1" ] || [ "${1%.conf}" != "$1" ]; then
  set -- iptv_cloud "$@"
fi

# allow the container to be started with `--user`
if [ "$1" = 'iptv_cloud' -a "$(id -u)" = '0' ]; then
  find . \! -user iptv_cloud -exec chown iptv_cloud '{}' +
  exec gosu iptv_cloud "$0" "$@"
fi

exec "$@"
