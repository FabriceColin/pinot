#!/bin/bash
# Run this with find, eg find -L ~/Documents/ -exec pinot-check-file.sh {} \;
if [ $# == 0 ]; then
  echo "Usage: $0 FILE"
  exit 1
fi

WHICH_SED=`which sed`
if [ $? != 0 ]; then
  echo "Couldn't find sed. Is the sed package installed ?"
  exit 1
fi
WHICH_PI=`which pinot-index`
if [ $? != 0 ]; then
  echo "Couldn't find pinot-index. Is the pinot package installed ?"
  exit 1
fi

for FILE in "$@" ;
do
  NO_SLASH_FILE=`echo $FILE | sed -e "s/\(.*\)\/$/\1/g"`
  pinot-index --check --db ~/.pinot/daemon/ "file://$NO_SLASH_FILE" >/dev/null 2>&1 \;
  if [ $? != 0 ]; then
    echo $NO_SLASH_FILE
  fi
done

exit 0
