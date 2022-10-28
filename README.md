```console
$ make && opt -S -passes=globaldce ~/cap.ll | ./go | opt -S -opaque-pointers -passes=gvn,mem2reg,dce,dse
```
