all:
	~/workshop/compiler/build/bin/clang -O0 -emit-llvm cfi.c -c -o cfi.bc -include cfi.h
	~/workshop/compiler/build/bin/clang++ -O0 -emit-llvm sample.cpp -c -o sample.bc -include cfi.h
	~/workshop/compiler/build/bin/llvm-link cfi.bc sample.bc -o exec.bc
	~/workshop/compiler/build/bin/llvm-dis exec.bc
	~/workshop/compiler/build/bin/opt -load ~/workshop/compiler/build/lib/LLVMCiCfi.so -llvm-ci-cfi -CFG_OPT=false < exec.bc > exec_nocfg.bc
	~/workshop/compiler/build/bin/llvm-dis exec_nocfg.bc
	~/workshop/compiler/build/bin/llc -filetype=obj exec_nocfg.bc
	~/workshop/compiler/build/bin/clang++ exec_nocfg.o -o exec_nocfg
	~/workshop/compiler/build/bin/opt -load ~/workshop/compiler/build/lib/LLVMCiCfi.so -llvm-ci-cfi -CFG_OPT=true < exec.bc > exec_cfg.bc
	~/workshop/compiler/build/bin/llvm-dis exec_cfg.bc
	~/workshop/compiler/build/bin/llc -filetype=obj exec_cfg.bc
	~/workshop/compiler/build/bin/clang++ exec_cfg.o -o exec_cfg

clean:
	rm *.ll *.bc exec_nocfg exec_cfg