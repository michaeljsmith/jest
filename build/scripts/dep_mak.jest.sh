#!/bin/bash

input_file=$1
intermediate_file_stem=$2
collated_file=${intermediate_file_stem}.collated
predeclared_file=${intermediate_file_stem}.predeclared
preprocessed_gen_file=${intermediate_file_stem}.preprocessed.gen
preprocessed_file=${intermediate_file_stem}.preprocessed
evaluated_gen_file=${intermediate_file_stem}.evaluated
evaluated_file=${intermediate_file_stem}.evaluated

imported_files=$(sed -n -e 's/^\s*import\s\+\(.*\).*$/\1/p' $input_file)

for imported_file in $imported_files; do
  echo -e "-include obj/$imported_file.jest.dep.mak "
done;
echo -e ""

echo -ne "imported_files="
for imported_file in $imported_files; do
  echo -ne "\\\\\\n  obj/$imported_file.jest.evaluated "
done;
echo -e ""
echo -e ""

echo -e "${collated_file}: ${input_file} \$(imported_files)"
echo -e "${predeclared_file}: ${collated_file}"
echo -e "${preprocessed_gen_file}: ${collated_file} ${predeclared_file}"
echo -e "${preprocessed_file}: ${preprocessed_gen_file}"
echo -e "${evaluated_gen_file}: ${preprocessed_file}"
echo -e "${evaluated_file}: ${evaluated_gen_file}"

