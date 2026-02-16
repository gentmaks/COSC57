define dso_local i32 @func(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 10, ptr %3, align 4
  store i32 15, ptr %4, align 4
  store i32 25, ptr %5, align 4
  store i32 5, ptr %3, align 4
  br label %6

6:                                                ; preds = %17, %1
  %7 = load i32, ptr %3, align 4
  %8 = load i32, ptr %2, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %18

10:                                               ; preds = %6
  %11 = load i32, ptr %3, align 4
  %12 = add nsw i32 %11, 1
  store i32 %12, ptr %3, align 4
  %13 = load i32, ptr %3, align 4
  %14 = icmp sgt i32 %13, 15
  br i1 %14, label %15, label %16

15:                                               ; preds = %10
  store i32 25, ptr %5, align 4
  br label %17

16:                                               ; preds = %10
  store i32 25, ptr %5, align 4
  br label %17

17:                                               ; preds = %16, %15
  br label %6, !llvm.loop !6

18:                                               ; preds = %6
  %19 = load i32, ptr %3, align 4
  call void @print(i32 noundef %19)
  call void @print(i32 noundef 15)
  call void @print(i32 noundef 25)
  ret i32 40
}

