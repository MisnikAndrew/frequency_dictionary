MAX_SIZE_MB="$1"
mkdir -p data

for ((SIZE_MB=1; SIZE_MB<=MAX_SIZE_MB; SIZE_MB++)); do
    SIZE_BYTES=$((SIZE_MB * 1024 * 1024))
    head -c "$SIZE_BYTES" pg.txt > "data/pg_${SIZE_MB}.txt"
    echo "Extracted ${SIZE_MB}MB to pg_${SIZE_MB}.txt"
done