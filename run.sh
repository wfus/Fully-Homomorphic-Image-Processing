make
rm -rf logs
mkdir logs
cd benchmark
python3 benchmark.py
python3 analyze.py > results.txt
