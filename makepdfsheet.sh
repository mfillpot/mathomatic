#!/bin/sh

htmlfile="$1"quickref.html
pdffile=quickref.pdf

echo Generating the Mathomatic Quick Reference Sheet PDF file...

echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 TRANSITIONAL//EN\">" >"$htmlfile"
echo '<html>' >>"$htmlfile"
echo '<head>' >>"$htmlfile"
echo '<title>Mathomatic Quick Reference Sheet</title>' >>"$htmlfile"
echo '</head>' >>"$htmlfile"
echo '<body>' >>"$htmlfile"
echo '<pre>' >>"$htmlfile"

./mathomatic -e "set html all" "help all main equations constants >>$htmlfile" || (rm "$htmlfile"; exit 1)

echo >>"$htmlfile"
echo '<strong>For more information, visit <a href="http://www.mathomatic.org">www.mathomatic.org</a></strong>' >>"$htmlfile"

echo '</pre>' >>"$htmlfile"
echo '</body>' >>"$htmlfile"
echo '</html>' >>"$htmlfile"

if htmldoc --webpage --format pdf --linkstyle plain --no-links -f "$pdffile" "$htmlfile"
then
	echo "$pdffile" successfully created.
	rm "$htmlfile"
	exit 0
else
	rm "$htmlfile"
	exit 1
fi
