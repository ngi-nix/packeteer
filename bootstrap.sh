#!/bin/sh
set -e

cd 3rdparty

for archive in archives/*.tar.bz2 archives/*.tbz2 ; do
  if test -f "$archive" ; then
    tar vxfj "$archive"
  fi
done
for archive in archives/*.tar.gz archives/*.tgz ; do
  if test -f "$archive" ; then
    tar vxfz "$archive"
  fi
done

cd -

autoreconf -siv
