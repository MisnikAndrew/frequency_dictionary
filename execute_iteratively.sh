for i in $(seq 1 $1)
do
    ./main_code "data/pg_${i}.txt" --fast-map
done

./main_code "pg.txt" --fast-map 