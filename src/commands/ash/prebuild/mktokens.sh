#!/bin/sh -

# The following is a list of tokens. The second column is nonzero if the
# token marks the end of a list. The third column is the name to print in
# error messages.

cat > /tmp/ka$$ <<\!
TEOF	1	end of file
TNL	0	newline
TSEMI	0	";"
TBACKGND	0	"&"
TAND	0	"&&"
TOR	0	"||"
TPIPE	0	"|"
TLP	0	"("
TRP	1	")"
TEND_CASE	1	";;"
TEND_B_QUOTE	1	"`"
TREDIR	0	redirection
TWORD	0	word
TIF	0	"if"
TTHEN	1	"then"
TELSE	1	"else"
TELIF	1	"elif"
TFI	1	"fi"
TWHILE	0	"while"
TUNTIL	0	"until"
TFOR	0	"for"
TDO	1	"do"
TDONE	1	"done"
TBEGIN	0	"{"
TEND	1	"}"
TCASE	0	"case"
TESAC	1	"esac"
!
exec > ../token.h
i=0
while read line
do
	set -$- $line
	echo "#define $1 $i"
	i=`expr $i + 1`
done < /tmp/ka$$
echo '
/* Array indicating which tokens mark the end of a list */
const char tokenEndList[] = {'
while read line
do
	set -$- $line
	echo "	$2, "
done < /tmp/ka$$
echo '};

char *const tokenName[] = {'
sed -e 's/"/\\"/g' \
	-e 's/[^	]*[		][	]*[^	]*[		][	]*\(.*\)/	"\1",/' \
	/tmp/ka$$
echo '};
'

i=0
go=
sed 's/"//g' /tmp/ka$$ |
	while read line
	do
		set -$- $line
		if [ "$1" = TIF ]
		then
			echo "#define KWD_OFFSET $i"
			echo
			echo "char *const parseKwd[] = {"
			go=true
		fi
		if [ "$go" ]
		then
			echo "	\"$3\","
		fi
		i=`expr $i + 1`
	done
echo '	0
};'

rm /tmp/ka$$
