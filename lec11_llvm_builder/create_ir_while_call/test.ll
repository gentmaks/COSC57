target triple = "x86_64-pc-linux-gnu"

declare i32 @read()

define i32 @test(i32 %0, i32 %1) {
  %m = alloca i32, align 4
  %n = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 %0, ptr %m, align 4
  store i32 %1, ptr %n, align 4
  %3 = call i32 @read()
  store i32 %3, ptr %a, align 4
  br label %4

4:                                                ; preds = %8, %2
  %5 = load i32, ptr %m, align 4
  %6 = load i32, ptr %n, align 4
  %7 = icmp slt i32 %5, %6
  br i1 %7, label %8, label %11

8:                                                ; preds = %4
  %9 = load i32, ptr %m, align 4
  %10 = add i32 %9, 10
  store i32 %10, ptr %m, align 4
  br label %4

11:                                               ; preds = %4
  %12 = load i32, ptr %m, align 4
  ret i32 %12
}
