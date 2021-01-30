rm -f sample.txt
mkdir -p tmp
cp tests/sample.txt tmp
cd tmp
wc -l sample.txt
