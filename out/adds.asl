; ModuleID = './lib/state.ll'
source_filename = "/nonexistent.ll"

@X0 = global i64 0
@X1 = global i64 0
@X2 = global i64 0
@X3 = global i64 0
@X4 = global i64 0
@X5 = global i64 0
@X6 = global i64 0
@X7 = global i64 0
@X8 = global i64 0
@X9 = global i64 0
@X10 = global i64 0
@X11 = global i64 0
@X12 = global i64 0
@X13 = global i64 0
@X14 = global i64 0
@X15 = global i64 0
@X16 = global i64 0
@X17 = global i64 0
@X18 = global i64 0
@X19 = global i64 0
@X20 = global i64 0
@X21 = global i64 0
@X22 = global i64 0
@X23 = global i64 0
@X24 = global i64 0
@X25 = global i64 0
@X26 = global i64 0
@X27 = global i64 0
@X28 = global i64 0
@X29 = global i64 0
@X30 = global i64 0
@X31 = global i64 0
@V0 = global i128 0
@V1 = global i128 0
@V2 = global i128 0
@V3 = global i128 0
@V4 = global i128 0
@V5 = global i128 0
@V6 = global i128 0
@V7 = global i128 0
@V8 = global i128 0
@V9 = global i128 0
@V10 = global i128 0
@V11 = global i128 0
@V12 = global i128 0
@V13 = global i128 0
@V14 = global i128 0
@V15 = global i128 0
@V16 = global i128 0
@V17 = global i128 0
@V18 = global i128 0
@V19 = global i128 0
@V20 = global i128 0
@V21 = global i128 0
@V22 = global i128 0
@V23 = global i128 0
@V24 = global i128 0
@V25 = global i128 0
@V26 = global i128 0
@V27 = global i128 0
@V28 = global i128 0
@V29 = global i128 0
@V30 = global i128 0
@V31 = global i128 0
@NF = global i1 false
@ZF = global i1 false
@CF = global i1 false
@VF = global i1 false
@PC = global i64 0

define void @root() {
entry:
  %__BranchTaken = alloca i1, align 1
  %nRW = alloca i1, align 1
  %BTypeNext = alloca i2, align 1
  store i1 false, i1* %__BranchTaken, align 1
  br label %0

0:                                                ; preds = %entry
  %Exp7__5 = alloca i64, align 8
  %1 = load i64, i64* @X2, align 4
  store i64 %1, i64* %Exp7__5, align 4
  br label %2

2:                                                ; preds = %0
  %Exp10__6 = alloca i64, align 8
  %3 = load i64, i64* @X3, align 4
  store i64 %3, i64* %Exp10__6, align 4
  br label %4

4:                                                ; preds = %2
  %If22__5 = alloca i1, align 1
  br label %5

5:                                                ; preds = %4
  %6 = load i64, i64* %Exp7__5, align 4
  %7 = load i64, i64* %Exp10__6, align 4
  %8 = add i64 %6, %7
  %9 = icmp eq i64 %8, 0
  br i1 %9, label %10, label %12

10:                                               ; preds = %5
  store i1 true, i1* %If22__5, align 1
  br label %11

11:                                               ; preds = %10
  br label %14

12:                                               ; preds = %5
  store i1 false, i1* %If22__5, align 1
  br label %13

13:                                               ; preds = %12
  br label %14

14:                                               ; preds = %13, %11
  br label %15

15:                                               ; preds = %14
  %If24__5 = alloca i1, align 1
  br label %16

16:                                               ; preds = %15
  %17 = load i64, i64* %Exp7__5, align 4
  %18 = load i64, i64* %Exp10__6, align 4
  %19 = add i64 %17, %18
  %20 = zext i64 %19 to i66
  %21 = load i64, i64* %Exp7__5, align 4
  %22 = zext i64 %21 to i66
  %23 = load i64, i64* %Exp10__6, align 4
  %24 = zext i64 %23 to i66
  %25 = add i66 %22, %24
  %26 = icmp eq i66 %20, %25
  br i1 %26, label %27, label %29

27:                                               ; preds = %16
  store i1 false, i1* %If24__5, align 1
  br label %28

28:                                               ; preds = %27
  br label %31

29:                                               ; preds = %16
  store i1 true, i1* %If24__5, align 1
  br label %30

30:                                               ; preds = %29
  br label %31

31:                                               ; preds = %30, %28
  br label %32

32:                                               ; preds = %31
  %If26__5 = alloca i1, align 1
  br label %33

33:                                               ; preds = %32
  %34 = load i64, i64* %Exp7__5, align 4
  %35 = load i64, i64* %Exp10__6, align 4
  %36 = add i64 %34, %35
  %37 = sext i64 %36 to i65
  %38 = load i64, i64* %Exp7__5, align 4
  %39 = sext i64 %38 to i65
  %40 = load i64, i64* %Exp10__6, align 4
  %41 = sext i64 %40 to i65
  %42 = add i65 %39, %41
  %43 = icmp eq i65 %37, %42
  br i1 %43, label %44, label %46

44:                                               ; preds = %33
  store i1 false, i1* %If26__5, align 1
  br label %45

45:                                               ; preds = %44
  br label %48

46:                                               ; preds = %33
  store i1 true, i1* %If26__5, align 1
  br label %47

47:                                               ; preds = %46
  br label %48

48:                                               ; preds = %47, %45
  br label %49

49:                                               ; preds = %48
  %50 = load i1, i1* %If26__5, align 1
  store i1 %50, i1* @VF, align 1
  br label %51

51:                                               ; preds = %49
  %52 = load i1, i1* %If24__5, align 1
  store i1 %52, i1* @CF, align 1
  br label %53

53:                                               ; preds = %51
  %54 = load i1, i1* %If22__5, align 1
  store i1 %54, i1* @ZF, align 1
  br label %55

55:                                               ; preds = %53
  %56 = load i64, i64* %Exp7__5, align 4
  %57 = load i64, i64* %Exp10__6, align 4
  %58 = add i64 %56, %57
  %59 = lshr i64 %58, 63
  %60 = trunc i64 %59 to i1
  store i1 %60, i1* @NF, align 1
  br label %61

61:                                               ; preds = %55
  %62 = load i64, i64* %Exp7__5, align 4
  %63 = load i64, i64* %Exp10__6, align 4
  %64 = add i64 %62, %63
  store i64 %64, i64* @X1, align 4
  br label %65

65:                                               ; preds = %61
  %66 = load i1, i1* %__BranchTaken, align 1
  %67 = load i64, i64* @PC, align 4
  %68 = add i64 %67, 4
  %69 = select i1 %66, i64 %67, i64 %68
  store i64 %69, i64* @PC, align 4
  ret void
}
