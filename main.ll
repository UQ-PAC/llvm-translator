@i64format = private constant [8 x i8] c"-> %ld\0A\00"

declare i32 @printf(ptr, ...)

define i32 @main() {
  store i64 59295976895241433, ptr @X2
  store i64 581559469347981653, ptr @X3

  call void @root()

  %x1 = load i64, ptr @X1
  %pc = load i64, ptr @PC

  %n = load i1, ptr @NF
  %z = load i1, ptr @ZF
  %c = load i1, ptr @CF
  %v = load i1, ptr @VF

  %n64 = zext i1 %n to i64
  %z64 = zext i1 %z to i64
  %c64 = zext i1 %c to i64
  %v64 = zext i1 %v to i64


  call i32 @printf(ptr @i64format, i64 %x1)
  call i32 @printf(ptr @i64format, i64 %n64)
  call i32 @printf(ptr @i64format, i64 %z64)
  call i32 @printf(ptr @i64format, i64 %c64)
  call i32 @printf(ptr @i64format, i64 %v64)
  call i32 @printf(ptr @i64format, i64 %pc)

  ret i32 121
}
