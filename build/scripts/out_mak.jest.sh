#!/bin/bash

output_file=$1
input_file=$2
dep_file=${input_file}.dep.mak

echo -e "-include ${dep_file}"
echo -e ""
echo -e "outputs+=${output_file}"
