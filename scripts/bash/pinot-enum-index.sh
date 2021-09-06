#!/bin/bash
# A script that enumerates files in an index created by Pinot
# and gives an estimate of how much disk space those take.

if [ $# == 0 ]; then
  echo "Usage: $0 INDEX"
  exit 1
fi

# delve might be called something else
DELVE="delve"

# Check programs we need are available
WHICH_DELVE=`which delve 2>/dev/null`
if [ $? != 0 ]; then
  WHICH_DELVE=`which xapian-delve 2>/dev/null`
  if [ $? != 0 ]; then
    echo "Couldn't find delve. Is the xapian-core package installed ?"
    exit 1
  else
    DELVE="xapian-delve"
  fi
fi
WHICH_DU=`which du 2>/dev/null`
if [ $? != 0 ]; then
  echo "Couldn't find du. Is the coreutils package installed ?"
  exit 1
fi
WHICH_DC=`which dc 2>/dev/null`
if [ $? != 0 ]; then
  echo "Couldn't find dc. Is the bc package installed ?"
  exit 1
fi

if [ ! -d "$1" ]; then
  echo "$1 is not a directory"
  exit 1
fi

# Remove existing files
rm -f "$1/urls.txt" "$1/filesizes.txt"

# Get a list of documents
DOCIDS=`$DELVE -t X-MetaSE-Doc "$1" | sed -e "s/\(.*\): \(.*\)/\2/g"`
if [ $? != 0 ]; then
  echo "Couldn't query database at $1"
fi

echo "Listing documents in index"
echo "0" >> "$1/filesizes.txt"
for DOCID in $DOCIDS ;
do
  # Skip documents with a scheme other than file
  FILENAME=`$DELVE -d -r $DOCID "$1" | grep "url=file" | sed -e "s/url=\(.*\):\/\///g"`
  #echo "File name is $FILENAME"
  if [ $? == 0 ] && [ ! -z "$FILENAME" ]; then
    FILESIZE=`du -b "$FILENAME" | sed -e "s/\([0-9]*\)\(.*\)/\1/g"`
    if [ ! -z "$FILESIZE" ]; then
      echo $FILESIZE >> "$1/filesizes.txt"
      echo "+" >> "$1/filesizes.txt"
    fi
    echo "$FILENAME" >> "$1/urls.txt"
  else
    # Dump documents with a scheme other than file
    URL=`$DELVE -d -r $DOCID "$1" | grep "url=" | sed -e "s/url=//g"`
    #echo "URL is $URL"
    if [ $? == 0 ] && [ ! -z "$URL" ]; then
        FILESIZE=`$DELVE -d -r $DOCID "$1" | grep "size=" | sed -e "s/size=//g"`
        if [ ! -z "$FILESIZE" ]; then
          echo $FILESIZE >> "$1/filesizes.txt"
          echo "+" >> "$1/filesizes.txt"
        fi
        echo "$URL" >> "$1/urls.txt"
    fi
  fi
done
echo "n" >> "$1/filesizes.txt"
echo "List is in $1/urls.txt"

echo "Summarizing disk usage for indexed documents"
TOTALSIZE=`dc --file="$1/filesizes.txt"`
echo "$TOTALSIZE bytes"

exit 0
