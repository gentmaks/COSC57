define dso_local i32 @func(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 10, ptr %3, align 4
  store i32 20, ptr %4, align 4
  store i32 20, ptr %5, align 4
  store i32 5, ptr %3, align 4
  br label %7

7:                                                ; preds = %18, %1
  %8 = load i32, ptr %3, align 4
  %9 = load i32, ptr %2, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %19

11:                                               ; preds = %7
  %12 = load i32, ptr %3, align 4
  %13 = add nsw i32 %12, 1
  store i32 %13, ptr %3, align 4
  %14 = load i32, ptr %3, align 4
  %15 = icmp sgt i32 %14, 20
  br i1 %15, label %16, label %17

16:                                               ; preds = %11
  store i32 25, ptr %5, align 4
  br label %18

17:                                               ; preds = %11
  store i32 25, ptr %5, align 4
  br label %18

18:                                               ; preds = %17, %16
  br label %7, !llvm.loop !6

19:                                               ; preds = %7
  %20 = load i32, ptr %3, align 4
  call void @print(i32 noundef %20)
  call void @print(i32 noundef 20)
  %21 = load i32, ptr %5, align 4
  call void @print(i32 noundef %21)
  %22 = add nsw i32 20, %21
  ret i32 %22
}

