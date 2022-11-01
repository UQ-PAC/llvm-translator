; ModuleID = './lib/state.ll'
source_filename = "/nonexistent.ll"

@X0 = internal global i64 0
@X1 = internal global i64 0
@X2 = internal global i64 0
@X3 = internal global i64 0
@X4 = internal global i64 0
@X5 = internal global i64 0
@X6 = internal global i64 0
@X7 = internal global i64 0
@X8 = internal global i64 0
@X9 = internal global i64 0
@X10 = internal global i64 0
@X11 = internal global i64 0
@X12 = internal global i64 0
@X13 = internal global i64 0
@X14 = internal global i64 0
@X15 = internal global i64 0
@X16 = internal global i64 0
@X17 = internal global i64 0
@X18 = internal global i64 0
@X19 = internal global i64 0
@X20 = internal global i64 0
@X21 = internal global i64 0
@X22 = internal global i64 0
@X23 = internal global i64 0
@X24 = internal global i64 0
@X25 = internal global i64 0
@X26 = internal global i64 0
@X27 = internal global i64 0
@X28 = internal global i64 0
@X29 = internal global i64 0
@X30 = internal global i64 0
@X31 = internal global i64 0
@V0 = internal global i128 0
@V1 = internal global i128 0
@V2 = internal global i128 0
@V3 = internal global i128 0
@V4 = internal global i128 0
@V5 = internal global i128 0
@V6 = internal global i128 0
@V7 = internal global i128 0
@V8 = internal global i128 0
@V9 = internal global i128 0
@V10 = internal global i128 0
@V11 = internal global i128 0
@V12 = internal global i128 0
@V13 = internal global i128 0
@V14 = internal global i128 0
@V15 = internal global i128 0
@V16 = internal global i128 0
@V17 = internal global i128 0
@V18 = internal global i128 0
@V19 = internal global i128 0
@V20 = internal global i128 0
@V21 = internal global i128 0
@V22 = internal global i128 0
@V23 = internal global i128 0
@V24 = internal global i128 0
@V25 = internal global i128 0
@V26 = internal global i128 0
@V27 = internal global i128 0
@V28 = internal global i128 0
@V29 = internal global i128 0
@V30 = internal global i128 0
@V31 = internal global i128 0
@NF = internal global i1 false
@ZF = internal global i1 false
@CF = internal global i1 false
@VF = internal global i1 false
@PC = internal global i64 0

define void @main() {
entry:
  %__BranchTaken = alloca i1, align 1
  %nRW = alloca i1, align 1
  %BTypeNext = alloca i2, align 1
  store i1 false, i1* %__BranchTaken, align 1
  br label %0

0:                                                ; preds = %entry
  %Exp4__5 = alloca i64, align 8
  %1 = load i64, i64* @X30, align 4
  store i64 %1, i64* %Exp4__5, align 4
  br label %2

2:                                                ; preds = %0
  store i2 0, i2* %BTypeNext, align 1
  br label %3

3:                                                ; preds = %2
  store i1 true, i1* %__BranchTaken, align 1
  br label %4

4:                                                ; preds = %3
  %Exp6__6 = alloca i1, align 1
  %5 = load i1, i1* %nRW, align 1
  store i1 %5, i1* %Exp6__6, align 1
  br label %6

6:                                                ; preds = %4
  br label %7

7:                                                ; preds = %6
  %8 = load i64, i64* %Exp4__5, align 4
  store i64 %8, i64* @PC, align 4
  br label %9

9:                                                ; preds = %7
  %10 = load i1, i1* %__BranchTaken, align 1
  %11 = load i64, i64* @PC, align 4
  %12 = add i64 %11, 4
  %13 = select i1 %10, i64 %11, i64 %12
  store i64 %13, i64* @PC, align 4
  ret void
}
