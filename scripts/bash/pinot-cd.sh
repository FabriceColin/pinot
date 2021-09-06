#!/bin/bash
# A "tagged cd" implementation
# Based on an idea by C. Scott Ananian (http://wiki.laptop.org/go/Experiments_with_unordered_paths)
# Run this script with ". /path/to/pinot-cd.sh"

MISSING_PROGRAM=1
MISSING_INDEX=1
MATCHES_COUNT=0

# Check programs we need are available
checkprograms()
{
  WHICH_BN=`which basename 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find basename. Is the coreutils package installed ?"
    return
  fi
  WHICH_CAT=`which cat 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find cat. Is the coreutils package installed ?"
    return
  fi
  WHICH_MKTEMP=`which mktemp 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find mktemp. Is the coreutils package installed ?"
    return
  fi
  WHICH_GREP=`which grep 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find grep. Is the grep package installed ?"
    return
  fi
  WHICH_SED=`which sed 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find sed. Is the sed package installed ?"
    return
  fi
  WHICH_PS=`which pinot-search 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find pinot-search. Is the pinot package installed ?"
    return
  fi

  MISSING_PROGRAM=0
}

# Check the daemon's index exists
checkindex()
{
  if [ ! -d "$HOME/.pinot/daemon" ]; then
    echo "Configure Pinot to crawl and index directories first."
    return
  fi

  MISSING_INDEX=0
}

# Run the actual search and get matches
runsearch()
{
  local TEMP_FILE=`mktemp`

  pinot-search -l xapian "$HOME/.pinot/daemon" "type:x-directory/normal $1" 2>/dev/null | grep "file://" | sed -e "s/file:\/\/\(.*\)/\1/g" >$TEMP_FILE 2>/dev/null

  local RESULTS=`cat $TEMP_FILE`
  MATCHES_COUNT=`cat $TEMP_FILE|wc -l`

  if [ $MATCHES_COUNT -eq 1 ]; then
    echo $RESULTS
    cd "$RESULTS"
  elif [ $MATCHES_COUNT -gt 1 ] && [ $2 -eq 1 ]; then
    echo "Matches for \"$1\":"
    cat $TEMP_FILE
  elif [ $MATCHES_COUNT -eq 0 ] && [ $2 -eq 1 ]; then
    echo "No match"
  fi
  \rm -f $TEMP_FILE >/dev/null 2>&1
}

# Prepares the query string
preparequery()
{
  local ARGS=""
  local LAST_ARG=""

  for ARG in `echo $@` ;
  do
    ARGS="$ARGS path:$ARG"
    LAST_ARG="$ARG"
  done

  if [ -z "$LAST_ARG" ]; then
    return
  fi

  PATHS_ONLY_STRING=$ARGS
  # Assume the last path is the name of the final directory
  PATHS_AND_FILE_STRING=`echo $ARGS | sed -e "s/path:$LAST_ARG/file:$LAST_ARG/g"`

  runsearch "$PATHS_AND_FILE_STRING" 1
  if [ $MATCHES_COUNT -eq 0 ]; then
    # That assumption doesn't seem correct
    runsearch "$PATHS_ONLY_STRING" 1
  fi
}

# Warn if not sourced
BASE_NAME=`basename $0`
if [ "$BASE_NAME" == "pinot-cd.sh" ]; then
  echo "Run this script with . $0"
  exit 1
fi

checkprograms
checkindex

if [ $# == 0 ]; then
  echo "Usage: $0 PATH1 PATH2..."
elif [ "$MISSING_PROGRAM" == 0 ] || [ "$MISSING_INDEX" == 0 ]; then
  if [ $# -eq 1 ] && [ "$1" == "-" ] && [ ! -z "$OLDPWD" ]; then
    cd "$OLDPWD"
  else
    IS_FULL_PATH=`echo "$@" | grep "/"`
    if [ ! -z "$IS_FULL_PATH" ]; then
      cd $@
    else
      preparequery $@
    fi
  fi
fi

