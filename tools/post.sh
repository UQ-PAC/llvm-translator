#!/bin/bash


opt -S -opaque-pointers -passes=inline,mergereturn,mem2reg,gvn,early-cse,simplifycfg,tailcallelim,simplifycfg,instcombine,reg2mem,gvn,dce
#opt -S -opaque-pointers -passes=inline,mergereturn,simplifycfg,mem2reg,dce,dse,sink,sccp,gvn,dse,dce,verify
#opt -S -opaque-pointers -passes=inline,mergereturn,gvn,mem2reg,dce,dse,sink,dse,mem2reg,dce,verify
