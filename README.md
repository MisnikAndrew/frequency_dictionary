# Подсчет частотного словаря на C++

Best code in main_cpde.cpp <br>

## TO START
0. Move pg.txt to ~/
1. Run ./cut_file_iteratively.sh K to cut pg.txt into files of 1MB, 2MB, ... K MB. <br>
2. Run ./build.sh && ./execute_iteratively.sh K to compile&launch for first K files <br> <br>

## Parameters
In execute_iteratively.sh You can change program parameters:<br>
1. --print-result to print results in sorted order<br>
2. --fast-map to use hand-written hash map. <br>

## Results parsing & visualizing
1. Results are printed into console <br>
2. You can copy them and put into results/results.txt <br>
3. Execute ./scripts/results_extractor.sh <YOUR_METHOD_NAME> to parse result times and sizes to results/<YOUR_METHOD_NAME>/
4. Add special metrics to sandbox.ipynb to build graphs


## TODO
1. Rewrite std::hash function with simple hash <br>
2. Use std::vector to make less allocations <br>
3. Rewrite FastUnorderedMap in more convenient way