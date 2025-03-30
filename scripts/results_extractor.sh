#!/bin/bash

result_sizes=()
time_taken=()
while IFS= read -r line; do
  result_size=$(echo "$line" | awk -F '|' '{ gsub(/[^0-9]/, "", $2); print $2 }')
  time_value=$(echo "$line" | awk -F '|' '{ gsub(/[^0-9.]/, "", $3); print $3 }')
  result_sizes+=("$result_size")
  time_taken+=("$time_value")
done < results/results.txt

IFS=','
FOLDER_NAME="results/${1}/"
mkdir $FOLDER_NAME
echo "${result_sizes[*]}" > "${FOLDER_NAME}results_size.txt"
echo "${time_taken[*]}" > "${FOLDER_NAME}results_times.txt"

echo "extracted results into ${FOLDER_NAME}"