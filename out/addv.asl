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

define void @root() {
entry:
  %__BranchTaken = alloca i1, align 1
  %nRW = alloca i1, align 1
  %BTypeNext = alloca i2, align 1
  store i1 false, i1* %__BranchTaken, align 1
  br label %stmt

stmt:                                             ; preds = %entry
  %Exp7__5 = alloca i128, align 8
  %0 = load i128, i128* @V1, align 4
  store i128 %0, i128* %Exp7__5, align 4
  br label %stmt1

stmt1:                                            ; preds = %stmt
  %Exp9__5 = alloca i128, align 8
  %1 = load i128, i128* @V2, align 4
  store i128 %1, i128* %Exp9__5, align 4
  br label %stmt2

stmt2:                                            ; preds = %stmt1
  %2 = load i128, i128* %Exp7__5, align 4
  %3 = lshr i128 %2, 96
  %4 = trunc i128 %3 to i32
  %5 = load i128, i128* %Exp9__5, align 4
  %6 = lshr i128 %5, 96
  %7 = trunc i128 %6 to i32
  %8 = add i32 %4, %7
  %9 = load i128, i128* %Exp7__5, align 4
  %10 = lshr i128 %9, 64
  %11 = trunc i128 %10 to i32
  %12 = load i128, i128* %Exp9__5, align 4
  %13 = lshr i128 %12, 64
  %14 = trunc i128 %13 to i32
  %15 = add i32 %11, %14
  %16 = load i128, i128* %Exp7__5, align 4
  %17 = lshr i128 %16, 32
  %18 = trunc i128 %17 to i32
  %19 = load i128, i128* %Exp9__5, align 4
  %20 = lshr i128 %19, 32
  %21 = trunc i128 %20 to i32
  %22 = add i32 %18, %21
  %23 = load i128, i128* %Exp7__5, align 4
  %24 = trunc i128 %23 to i32
  %25 = load i128, i128* %Exp9__5, align 4
  %26 = trunc i128 %25 to i32
  %27 = add i32 %24, %26
  %28 = zext i32 %22 to i64
  %29 = zext i32 %27 to i64
  %30 = shl i64 %28, 32
  %31 = or i64 %30, %29
  %32 = zext i32 %15 to i96
  %33 = zext i64 %31 to i96
  %34 = shl i96 %32, 64
  %35 = or i96 %34, %33
  %36 = zext i32 %8 to i128
  %37 = zext i96 %35 to i128
  %38 = shl i128 %36, 96
  %39 = or i128 %38, %37
  store i128 %39, i128* @V0, align 4
  br label %end

end:                                              ; preds = %stmt2
  %40 = load i1, i1* %__BranchTaken, align 1
  %41 = load i64, i64* @PC, align 4
  %42 = add i64 %41, 4
  %43 = select i1 %40, i64 %41, i64 %42
  store i64 %43, i64* @PC, align 4
  ret void
}
