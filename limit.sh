#!/bin/bash
for f in *.uniq.txt
do echo "Processing $f file.."
while IFS= read -r file_line
do
  value=$(echo "$file_line"|awk '{print $1}')
  if [ "$value" -gt "1" ]; then
  echo "$file_line" >> "$f.trim.txt"
  else break
  fi
done < "$f"
done
