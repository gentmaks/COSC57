define dso_local i32 @func(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 10, ptr %3, align 4
  store i32 20, ptr %4, align 4
  store i32 30, ptr %5, align 4
  %6 = load i32, ptr %2, align 4
  %7 = icmp slt i32 10, %6
  br i1 %7, label %8, label %9

8:                                                ; preds = %1
  store i32 30, ptr %4, align 4
  br label %10

9:                                                ; preds = %1
  store i32 30, ptr %4, align 4
  br label %10

10:                                               ; preds = %9, %8
  ret i32 40
}

