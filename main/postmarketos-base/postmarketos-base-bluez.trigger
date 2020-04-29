#!/bin/sh

# shellcheck disable=SC1091
. /etc/deviceinfo
# shellcheck disable=SC2154
sed -i "s|#Name = BlueZ|Name = $deviceinfo_name|" "$1"
