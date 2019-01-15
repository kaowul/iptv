#!/bin/sh
set -e

USER=iptv

# first arg is `-f` or `--some-option`
# or first arg is `something.conf`
if [ "${1#-}" != "$1" ] || [ "${1%.conf}" != "$1" ]; then
  set -- streamer_service "$@"
fi

# allow the container to be started with `--user`
if [ "$1" = 'streamer_service' -a "$(id -u)" = '0' ]; then
  find . \! -user ${USER} -exec chown ${USER} '{}' +
  exec gosu ${USER} "$0" "$@"
fi

exec "$@"
