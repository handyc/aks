#!/bin/sh

for i in ./unsorted.*.master.txt
do
cat "$i" | sort | uniq -c | sort -rn | awk '{print $1 " " $2}' > "$i".uniq.txt
echo "Created unique sorted master for $i"
done
echo "Completed sorting of masters."
