; ModuleID = 'minic_module'
source_filename = "minic_module"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_1 = alloca i32, align 4
  %b_2 = alloca i32, align 4
  %c_3 = alloca i32, align 4
  %papanewguinea_4 = alloca i32, align 4
  %a_5 = alloca i32, align 4
  %b_6 = alloca i32, align 4
  %c_7 = alloca i32, align 4
  %a_8 = alloca i32, align 4
  %b_9 = alloca i32, align 4
  %a_10 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  store i32 %0, ptr %i_0, align 4
  %retval.addr = alloca i32, align 4
  store i32 10, ptr %a_1, align 4
  store i32 10, ptr %b_2, align 4
  store i32 10, ptr %c_3, align 4
  store i32 30, ptr %a_5, align 4
  store i32 30, ptr %b_6, align 4
  store i32 30, ptr %c_7, align 4
  store i32 20, ptr %a_8, align 4
  store i32 20, ptr %b_9, align 4
  %1 = load i32, ptr %a_8, align 4
  %2 = icmp slt i32 %1, 30
  br i1 %2, label %if_t, label %if_f

ret:                                              ; preds = %after_ret, %if_f
  %retval = load i32, ptr %retval.addr, align 4
  ret i32 %retval

if_t:                                             ; preds = %entry
  store i32 20, ptr %a_10, align 4
  br label %if_f

if_f:                                             ; preds = %if_t, %entry
  store i32 1, ptr %retval.addr, align 4
  br label %ret

after_ret:                                        ; No predecessors!
  br label %ret
}
