#!/bin/bash

opt -S -opaque-pointers -passes=inline,mergereturn,simplifycfg,sccp,dce,dse,verify
#opt -S -opaque-pointers -passes=inline,mergereturn,simplifycfg,mem2reg,dce,dse,sink,sccp,gvn,dse,dce,verify
#opt -S -opaque-pointers -passes=inline,mergereturn,gvn,mem2reg,dce,dse,sink,dse,mem2reg,dce,verify
