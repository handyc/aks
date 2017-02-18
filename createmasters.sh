#!/bin/sh

COUNTER=1
while [  $COUNTER -lt 9 ]; do
####### create dictionary files for all texts
echo "Now processing ngrams of length $COUNTER..."

for f in ./*.$COUNTER.ngram
do
cat "$f" >> ./masters/unsorted.$COUNTER.master.txt
echo "Added $f to master unsorted list"
done

COUNTER=$((COUNTER+1))
done

## finished creating unsorted master lists, now created unique sorted masters
echo "Master lists have been created."


