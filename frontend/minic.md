# Mini C specification:

```
There are no comments in miniC
All values in miniC are of type int.
The program starts with two extern declarations for print and read as specified below:
The print function returns void and takes a single int parameter 
The read function doesn’t take any parameters and returns an int
The program has only one function that always returns an int and takes 0 or 1 parameter (int).
miniC has the following types of statements from C:
If 
If-else
While
Block
Assignment
RHS of an assignment may be a call to a read function
Call to a print function
Return 
A return value can be a constant, a variable, or a binary/unary arithmetic expression.
A return statement may appear anywhere in the body of the function.
miniC has the following types of expressions:
All expressions only have 2 operands
Operands can be integer constants or variables
Arithmetic expressions have operators: +, -, *, /
Relational expressions have operators: ==, !=, >, <, >=, <=
Operands to relational expressions can be arithmetic expressions
Unary expression: -
Declarations:
Each declaration statement declares only 1 variable
All declaration statements in a block statement are at the start of the block
Nested block statements can have declarations that are local to that block
```

# Mini C example
```
miniC
/*miniC language does not allow any comments; DO NOT USE THIS FILE TO TEST YOUR COMPILER */

/* This file gives you an example program in miniC language.

 * We focus on compiling a program in a single file using the following format:

 * A program in miniC contains only a single function definition. You can name this function anything other 

 * than main, print, or read. It has two extern function declarations

 * for print and read that we will use to print and read values without

 * explicitly calling printf and scanf. The extern declarations of these functions are always

 *in the order given below.

 * I have included comments with individual lines of code

 * to help you understand the structure of code in more detail.

*/

extern void print(int); // function to print a value

extern int read(); // function to read a value

 

/* In miniC we only use interger types.

* All local variables and parameters are integers.

* Return value of the function is also an integer.

*/

int func(int a){ //Can only have 0 or 1 parameter

        // All local variables are declared at the beginning of a 

        // code block and each variable is declared

        // separately in a new line.

        int loc1;

        int loc2;

        int i;

        int val;

 

        loc1 = a + 10; // All arithmetic operations will have only 2 operands

        loc2 = a * 10; // Arithmetic operations that are allowed are: +, -, *, / 

        val  = read();

 

        // Conditions in if and while statements will use only  >, <, ==, >=, <=, != operators. We do not use logical operators (&&, ||, !)

        if (loc1 > loc2) { // Body of an if statement can be a single statement or a block statement

                print(loc1); // Only functions you can call are print and read. We do not include system calls.

        }

        else

                print(loc2);

        i = 0;

        while (i < loc1){ // Body of a while loop can be a single statement or a block statement

                int j;

                i = i + val;

                j = loc2 + i;

                loc2 = j;

        }

        return(loc1 + loc2); // A return can also return a constant or a variable

}
```