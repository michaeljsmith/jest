#!/bin/bash

output_file=$1
intermediate_file_stem=$2
evaluated_file=${intermediate_file_stem}.evaluated
dep_file=${intermediate_file_stem}.dep.mak

echo -e "-include ${dep_file}"
echo -e ""
echo -e "${output_file}: ${evaluated_file}"
echo -e "\tcp ${evaluated_file} ${output_file}"
echo -e ""
echo -e "outputs+=${output_file}"
