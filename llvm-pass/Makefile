all:
	~/workshop/compiler/build/bin/clang -O0 -emit-llvm cfi.c -c -o cfi.bc -include cfi.h
	~/workshop/compiler/build/bin/clang++ -O0 -emit-llvm sample.cpp -c -o sample.bc -include cfi.h
	~/workshop/compiler/build/bin/llvm-link cfi.bc sample.bc -o exec.bc
	~/workshop/compiler/build/bin/llvm-dis exec.bc
	#~/workshop/compiler/build/bin/llc -filetype=obj exec.bc
	#~/workshop/compiler/build/bin/clang++ exec.o -o exec
	~/workshop/compiler/build/bin/opt -load ~/workshop/compiler/build/lib/LLVMCiCfi.so -llvm-ci-cfi < exec.bc > exec_rev.bc
	~/workshop/compiler/build/bin/llvm-dis exec_rev.bc
	~/workshop/compiler/build/bin/llc -filetype=obj exec_rev.bc
	~/workshop/compiler/build/bin/clang++ exec_rev.o -o exec_rev

clean:
	rm *.ll *.bc exec exec_rev