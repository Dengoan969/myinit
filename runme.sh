rm -f config*.txt
echo /usr/bin/bc -l $PWD/in1.txt $PWD/out1.txt >> config.txt
echo /usr/bin/sleep 5 $PWD/in2.txt $PWD/out2.txt >> config.txt
echo /usr/bin/bc -l $PWD/in3.txt  $PWD/out3.txt >> config.txt
echo /usr/bin/sleep 2 $PWD/in4.txt $PWD/out4.txt >> config2.txt

rm -f out* /tmp/myinit*
rm -f myinit.o
rm -f myinit
make

./myinit $PWD/config.txt
sleep 1

echo "myinit and children on start"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo

sleep 1

echo "myinit and children with restarted processes:"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo

cp config2.txt config.txt

pkill -SIGHUP myinit
sleep 2

echo "myinit and children with new config:"
pgrep -a myinit
pgrep -a -P $( pgrep myinit )
echo

pkill -SIGINT myinit
sleep 1

echo "myinit after SIGINT:"
pgrep -a myinit
echo