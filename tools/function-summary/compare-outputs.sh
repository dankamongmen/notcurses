#/usr/bin/env bash
#
# compares the changes betwee two output folders

PATH1=$1
PATH2=$2

if [[ -z $PATH1 ]]; then
	echo "It's necessary to provide two paths to compare. 0 paths provided"
	exit 1
fi

if [[ ! -d $PATH1 ]]; then
	echo "The first path doesn't exist."
	exit 1
fi

if [[ -z $PATH2 ]]; then
	echo "It's necessary to provide two paths to compare. Only 1 path provided."
	exit 1
fi

if [[ ! -d $PATH2 ]]; then
	echo "The second path doesn't exist."
	exit 1
fi


# this line is to activate diff syntax highlighting
echo -e "0d0\n"

echo -e "Differences between files:\n=========================="
#DIFFLIST=$(diff -qr "$PATH1" "$PATH2"  | grep ^Files | cut -d' ' -f 2,4)
DIFFLIST=$(diff -qr "$PATH1" "$PATH2"  | grep ^Files | cut -d' ' -f 2,4 | grep -v STATS)
echo "$DIFFLIST"

echo -e "\nbroken down:\n============"

OLDIFS=$IFS
IFS=$'\n'
for d in $DIFFLIST; do
	d1=$(echo $d | cut -d' ' -f1)
	d2=$(echo $d | cut -d' ' -f2)

	echo -e "\ndiff $d1 $d2:"
	diff $d1 $d2
done


echo -e "\nOnly in one path:\n================="
UNIQUELIST=$(diff -qr "$PATH1" "$PATH2"  | grep ^Only)
echo "$UNIQUELIST"

echo -e "\ndisplay contents:\n================="

for u in $UNIQUELIST; do
	u1=$(echo $u | cut -d' ' -f3| tr ':' '/' )
	u2=$(echo $u | cut -d' ' -f4)
	path="${u1}${u2}"
	echo -e "\ncat $path:\n-------------------"
	cat $path
done


IFS=$OLDIFS
