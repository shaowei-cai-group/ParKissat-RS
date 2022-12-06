### Compile
```
make clean; make
```


### Run

$instance: input cnf

$threads: number of threads

$time-limit: maximum running time


```
time ./painless-mcomsps -c=(($threads-1)) -shr-sleep=500000 -shr-lit=1500 $instance -t=$time-limit -initshuffle
```

The script to run parkiss with 32 cores is given as `run_parkissat.sh'.





