; ModuleID = 'minic_module'
source_filename = "minic_module"
target triple = "x86_64-pc-linux-gnu"

declare void @print(i32)

declare i32 @read()

define i32 @func(i32 %0) {
entry:
  %a_1 = alloca i32, align 4
  %b_2 = alloca i32, align 4
  %i_0 = alloca i32, align 4
  store i32 %0, ptr %i_0, align 4
  %retval.addr = alloca i32, align 4
  store i32 5, ptr %a_1, align 4
  store i32 2, ptr %b_2, align 4
  %1 = load i32, ptr %a_1, align 4
  %2 = load i32, ptr %i_0, align 4
  %3 = icmp slt i32 %1, %2
  br i1 %3, label %if_t, label %if_f

ret:                                              ; preds = %after_ret, %if_end
  %retval = load i32, ptr %retval.addr, align 4
  ret i32 %retval

if_t:                                             ; preds = %entry
  br label %while_cond

if_f:                                             ; preds = %entry
  %4 = load i32, ptr %b_2, align 4
  %5 = load i32, ptr %i_0, align 4
  %6 = icmp slt i32 %4, %5
  br i1 %6, label %if_t1, label %if_f2

while_cond:                                       ; preds = %while_body, %if_t
  %7 = load i32, ptr %b_2, align 4
  %8 = load i32, ptr %i_0, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %while_body, label %while_exit

while_body:                                       ; preds = %while_cond
  %10 = load i32, ptr %b_2, align 4
  %11 = add i32 %10, 20
  store i32 %11, ptr %b_2, align 4
  br label %while_cond

while_exit:                                       ; preds = %while_cond
  %12 = load i32, ptr %b_2, align 4
  %13 = add i32 10, %12
  store i32 %13, ptr %a_1, align 4
  br label %if_end

if_t1:                                            ; preds = %if_f
  %14 = load i32, ptr %a_1, align 4
  store i32 %14, ptr %b_2, align 4
  br label %if_f2

if_f2:                                            ; preds = %if_t1, %if_f
  br label %if_end

if_end:                                           ; preds = %if_f2, %while_exit
  store i32 1, ptr %retval.addr, align 4
  br label %ret

after_ret:                                        ; No predecessors!
  br label %ret
}
