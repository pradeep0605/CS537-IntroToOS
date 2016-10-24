if [ $# -ne 2 ]; then
echo "Please give the input file to be processed and number of processes"
exit
fi

grep "KERNEL" $1 | cut -d ":" -f 3 > $1.tmp

i=0
while [ $i -le $2 ]; do
grep P$i $1.tmp | sed "s/P$i;/ /" > $1_P$i.plt 
i=$((i+1))
done
