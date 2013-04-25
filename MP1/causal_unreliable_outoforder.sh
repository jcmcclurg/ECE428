python causaltest.py 2 1 > one.txt & python causaltest.py 2 1 > two.txt & python causaltest.py 2 1 > three.txt

pid1=`head -n 1 one.txt | sed -r 's#[^0-9]##g'`
cat one.txt | sed -n -r "s#Sending message \[(.*)\]#\1#p" > oneToAll.txt
cat two.txt | sed -n -r "s#Received \[(.*)\] from $pid1#\1#p" > twoFromOne.txt
cat three.txt | sed -n -r "s#Received \[(.*)\] from $pid1#\1#p" > threeFromOne.txt
diff oneToAll.txt twoFromOne.txt > d12.txt
diff oneToAll.txt threeFromOne.txt > d13.txt
echo "Last message from one is:"
tail -n 1 oneToAll.txt

echo "Two's undelivered messages from one:"
cat d12.txt

echo "Three's undelivered messages from one:"
cat d13.txt
echo

pid2=`head -n 1 two.txt | sed -r 's#[^0-9]##g'`
cat two.txt | sed -n -r "s#Sending message \[(.*)\]#\1#p" > twoToAll.txt
cat one.txt | sed -n -r "s#Received \[(.*)\] from $pid2#\1#p" > oneFromTwo.txt
cat three.txt | sed -n -r "s#Received \[(.*)\] from $pid2#\1#p" > threeFromTwo.txt
diff twoToAll.txt oneFromTwo.txt > d21.txt
diff twoToAll.txt threeFromTwo.txt > d23.txt
echo "Last message from two is:"
tail -n 1 twoToAll.txt

echo "One's undelivered messages from two:"
cat d21.txt

echo "Three's undelivered messages from two:"
cat d23.txt
echo

pid3=`head -n 1 three.txt | sed -r 's#[^0-9]##g'`
cat three.txt | sed -n -r "s#Sending message \[(.*)\]#\1#p" > threeToAll.txt
cat one.txt | sed -n -r "s#Received \[(.*)\] from $pid3#\1#p" > oneFromThree.txt
cat two.txt | sed -n -r "s#Received \[(.*)\] from $pid3#\1#p" > twoFromThree.txt
diff threeToAll.txt oneFromThree.txt > d31.txt
diff threeToAll.txt twoFromThree.txt > d32.txt
echo "Last message from three is:"
tail -n 1 threeToAll.txt

echo "One's undelivered messages from three:"
cat d31.txt

echo "Two's undelivered messages from three:"
cat d32.txt
