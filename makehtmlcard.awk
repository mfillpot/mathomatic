# Convert TAB delimited Mathomatic help file to HTML.
# See "makehtmlcard.sh".
# Usage awk -F"\t" -f makehtmlcard.awk infile.txt >outfile.html
# Credit goes to John Blommers (http://www.blommers.org) for starting this awk file and for the cheat sheet idea.

BEGIN {
	print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 TRANSITIONAL//EN\">"
	print "<html>"
	print "<head>"
}

NR==1 {
	print "<title>Mathomatic Quick Reference Card</title>"
	print "</head>"
	print "<body>"
	print "<table cellpadding=\"4\" border=\"3\" summary=\"Mathomatic Quick Reference Card\">"
	print "<tr bgcolor=\"#2648fe\">" "<th colspan=\"3\">" "<font color=\"white\">" $1 "</font>" "</th>" "</tr>"
}

NR==2 {
	print "<tr>"
	print "<th>" $1 "</th>"
	print "<th>" $2 "</th>"
	print "<th>" $3 "</th>"
	print "</tr>"
}

NR>2 {
	print "<tr>"
	print "<td nowrap=\"nowrap\">" $1 "</td>" 
	print "<td nowrap=\"nowrap\">" $2 "</td>"
	print "<td nowrap=\"nowrap\">" $3 "</td>"
	print "</tr>"
}

END {
	print "</table>"
	print "<br clear=all>"
#	print "<p>"
	print "<font size=\"+1\">"
	print "Anything enclosed by straight brackets <b>[like this]</b> means it is optional and may be omitted."
	print "</font>"
#	print "<p>"
#	print "<font size=\"+1\">"
#	print "To select an equation space and make it the current equation, type the equation number at the main prompt.<br>"
#	print "To solve the current equation, type the variable name at the main prompt or use the solve command."
#	print "</font>"
	print "<p>"
	print "<font size=\"+1\">"
	print "<strong>For more information, visit <a href=\"http://www.mathomatic.org\">www.mathomatic.org</a></strong>"
	print "</font>"
	print "</body>"
	print "</html>"
}
