#!/bin/sh

set -e

textfile="$1"quickrefcard.txt
trap "echo Removing \"$textfile\"; rm -f \"$textfile\"" EXIT
echo Creating Mathomatic Quick Reference Card named: "$1"quickrefcard.html
./mathomatic -e "help table >$textfile"
echo Running awk...
awk -F"\t" -f makehtmlcard.awk "$textfile" >"$1"quickrefcard.html
echo Created "$1"quickrefcard.html
echo Use print to PDF file in Firefox to create the PDF quick reference card.

# pisa "$1"quickrefcard.html "$1"quickrefcard.pdf
