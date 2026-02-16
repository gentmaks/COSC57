
declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
  %p = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 %0, ptr %p, align 4
  %2 = call i32 @read()
  store i32 %2, ptr %a, align 4
  %3 = load i32, ptr %a, align 4
  %4 = load i32, ptr %p, align 4
  %5 = icmp sgt i32 %3, %4
  br i1 %5, label %6, label %8

6:                                                ; preds = %1
  %7 = load i32, ptr %p, align 4
  ret i32 %7

8:                                                ; preds = %1
  %9 = load i32, ptr %a, align 4
  ret i32 %9
}
