for i in $(seq 1 $1)
do
    ./baseline "data/pg_${i}.txt"
done