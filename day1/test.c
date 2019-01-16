int f(int x){
    int result  = (x/42);
    return result;
}

// clang -Xclang -ast-dump -fsyntax-only test.c

/*
TranslationUnitDecl 0x558fdece8bb8 <<invalid sloc>> <invalid sloc>
|-TypedefDecl 0x558fdece9150 <<invalid sloc>> <invalid sloc> implicit __int128_t '__int128'
| `-BuiltinType 0x558fdece8e50 '__int128'
|-TypedefDecl 0x558fdece91b8 <<invalid sloc>> <invalid sloc> implicit __uint128_t 'unsigned __int128'
| `-BuiltinType 0x558fdece8e70 'unsigned __int128'
|-TypedefDecl 0x558fdece9488 <<invalid sloc>> <invalid sloc> implicit __NSConstantString 'struct __NSConstantString_tag'
| `-RecordType 0x558fdece9290 'struct __NSConstantString_tag'
|   `-Record 0x558fdece9208 '__NSConstantString_tag'
|-TypedefDecl 0x558fdece9520 <<invalid sloc>> <invalid sloc> implicit __builtin_ms_va_list 'char *'
| `-PointerType 0x558fdece94e0 'char *'
|   `-BuiltinType 0x558fdece8c50 'char'
|-TypedefDecl 0x558fdece97e8 <<invalid sloc>> <invalid sloc> implicit __builtin_va_list 'struct __va_list_tag [1]'
| `-ConstantArrayType 0x558fdece9790 'struct __va_list_tag [1]' 1 
|   `-RecordType 0x558fdece9600 'struct __va_list_tag'
|     `-Record 0x558fdece9570 '__va_list_tag'
`-FunctionDecl 0x558fded457f8 <test.c:1:1, line:4:1> line:1:5 f 'int (int)'
  |-ParmVarDecl 0x558fdece9850 <col:7, col:11> col:11 used x 'int'
  `-CompoundStmt 0x558fded45a70 <col:13, line:4:1>
    |-DeclStmt 0x558fded45a00 <line:2:5, col:25>
    | `-VarDecl 0x558fded458f8 <col:5, col:24> col:9 used result 'int' cinit
    |   `-ParenExpr 0x558fded459e0 <col:19, col:24> 'int'
    |     `-BinaryOperator 0x558fded459b8 <col:20, col:22> 'int' '/'
    |       |-ImplicitCastExpr 0x558fded459a0 <col:20> 'int' <LValueToRValue>
    |       | `-DeclRefExpr 0x558fded45958 <col:20> 'int' lvalue ParmVar 0x558fdece9850 'x' 'int'
    |       `-IntegerLiteral 0x558fded45980 <col:22> 'int' 42
    `-ReturnStmt 0x558fded45a58 <line:3:5, col:12>
      `-ImplicitCastExpr 0x558fded45a40 <col:12> 'int' <LValueToRValue>
        `-DeclRefExpr 0x558fded45a18 <col:12> 'int' lvalue Var 0x558fded458f8 'result' 'int'
*/

// clang -S -emit-llvm test.c 

/*
; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @f(i32 %x) #0 {
entry:
  %x.addr = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 %x, i32* %x.addr, align 4
  %0 = load i32, i32* %x.addr, align 4
  %div = sdiv i32 %0, 42
  store i32 %div, i32* %result, align 4
  %1 = load i32, i32* %result, align 4
  ret i32 %1
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 7.0.0 (trunk) (llvm/trunk 333182)"}
*/