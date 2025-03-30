for i in $(seq 1 $1)
do
    ./baseline_dump_read "data/pg_${i}.txt" --fast-map
done

./baseline_dump_read "pg.txt" --fast-map