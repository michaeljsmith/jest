#!/bin/bash

input_file=$1
intermediate_file_stem=$2
preprocessed_file=${intermediate_file_stem}.preprocessed

imported_files=$(sed -n -e 's/^\s*import\s\+\(.*\).*$/\1/p' $input_file)

echo -ne "imported_files="
for imported_file in $imported_files; do
  echo -ne "\\\\\\n  $imported_file.jest "
done;
echo -e ""
echo -e ""

echo -e "${preprocessed_file}: \$(imported_files)"
