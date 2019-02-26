; ModuleID = 'exec_cfg.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.CFI_STRUCT = type { i64, i64, %struct.CFI_STRUCT* }
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
%struct.ST = type { i32, void (i32)* }
%class.Hello = type { i32 (...)**, void ()*, void ()*, { i64, i64 } }

$_ZN5HelloC2EPFvvE = comdat any

$_ZN5Hello5ptofnEv = comdat any

$_ZN5Hello5vFuncEv = comdat any

$_ZTV5Hello = comdat any

$_ZTI5Hello = comdat any

$_ZTS5Hello = comdat any

@CFG_LENGTH = dso_local global i32 10, align 4
@passCounter = dso_local global i32 0, align 4
@cfi_hash_table = dso_local global [1000000 x %struct.CFI_STRUCT*] zeroinitializer, align 16
@_cfi_preinit = dso_local global void ()* @cfi_init, section ".preinit_array", align 8
@CFG_TABLE = external global [0 x i32*], section "cfg_label_data", align 8
@_cfi_fini = dso_local global void ()* @cfi_free, section ".fini_array", align 8
@stderr = external dso_local global %struct._IO_FILE*, align 8
@.str.1 = private unnamed_addr constant [19 x i8] c"Pass Counter: %ld\0A\00", align 1
@llvm.used = appending global [3 x i8*] [i8* bitcast (i32* @CFG_LENGTH to i8*), i8* bitcast (void ()** @_cfi_preinit to i8*), i8* bitcast (void ()** @_cfi_fini to i8*)], section "llvm.metadata"
@.str = private unnamed_addr constant [41 x i8] c"[pCall-CFI] Error at %ld target to 0x%x\0A\00", align 1
@st_arr = dso_local global [2 x %struct.ST] [%struct.ST { i32 10, void (i32)* @_Z5CallDi }, %struct.ST { i32 20, void (i32)* @_Z5CallFi }], align 16
@gl = dso_local global void ()* @_Z5CallEv, align 8
@_ZTV5Hello = linkonce_odr dso_local unnamed_addr constant { [3 x i8*] } { [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI5Hello to i8*), i8* bitcast (void (%class.Hello*)* @_ZN5Hello5vFuncEv to i8*)] }, comdat, align 8
@_ZTI5Hello = linkonce_odr dso_local constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([7 x i8], [7 x i8]* @_ZTS5Hello, i32 0, i32 0) }, comdat, align 8
@_ZTVN10__cxxabiv117__class_type_infoE = external dso_local global i8*
@_ZTS5Hello = linkonce_odr dso_local constant [7 x i8] c"5Hello\00", comdat, align 1
@CFG_TABLE.1 = constant [10 x i32*] [i32* inttoptr (i32 53415718 to i32*), i32* bitcast (void (%class.Hello*)* @_ZN5Hello5ptofnEv to i32*), i32* inttoptr (i32 53415718 to i32*), i32* bitcast (void (%class.Hello*)* @_ZN5Hello5vFuncEv to i32*), i32* inttoptr (i32 133337585 to i32*), i32* bitcast (void (i32)* @_Z5CallDi to i32*), i32* inttoptr (i32 133337585 to i32*), i32* bitcast (void (i32)* @_Z5CallFi to i32*), i32* inttoptr (i32 133337585 to i32*), i32* bitcast (void (i32)* @_Z5CallGi to i32*)]

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @cfi_init() #0 {
entry:
  %i = alloca i64, align 8
  store i64 0, i64* %i, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, i64* %i, align 8
  %1 = load i32, i32* @CFG_LENGTH, align 4
  %conv = zext i32 %1 to i64
  %cmp = icmp ult i64 %0, %conv
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, i64* %i, align 8
  %arrayidx = getelementptr inbounds [10 x i32*], [10 x i32*]* @CFG_TABLE.1, i64 0, i64 %2
  %3 = load i32*, i32** %arrayidx, align 8
  %4 = ptrtoint i32* %3 to i64
  %5 = load i64, i64* %i, align 8
  %add = add i64 %5, 1
  %arrayidx2 = getelementptr inbounds [10 x i32*], [10 x i32*]* @CFG_TABLE.1, i64 0, i64 %add
  %6 = load i32*, i32** %arrayidx2, align 8
  %7 = ptrtoint i32* %6 to i64
  call void @cfi_hash_insert(i64 %4, i64 %7)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %8 = load i64, i64* %i, align 8
  %add3 = add i64 %8, 2
  store i64 %add3, i64* %i, align 8
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @cfi_hash_insert(i64 %call_point, i64 %call_target) #0 {
entry:
  %call_point.addr = alloca i64, align 8
  %call_target.addr = alloca i64, align 8
  %cfi_hash_key = alloca i64, align 8
  %item = alloca %struct.CFI_STRUCT*, align 8
  %temp = alloca %struct.CFI_STRUCT*, align 8
  store i64 %call_point, i64* %call_point.addr, align 8
  store i64 %call_target, i64* %call_target.addr, align 8
  %0 = load i64, i64* %call_point.addr, align 8
  %1 = load i64, i64* %call_target.addr, align 8
  %xor = xor i64 %0, %1
  %rem = urem i64 %xor, 1000000
  store i64 %rem, i64* %cfi_hash_key, align 8
  %call = call noalias i8* @malloc(i64 24) #6
  %2 = bitcast i8* %call to %struct.CFI_STRUCT*
  store %struct.CFI_STRUCT* %2, %struct.CFI_STRUCT** %item, align 8
  %3 = load i64, i64* %call_point.addr, align 8
  %4 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %item, align 8
  %iCall = getelementptr inbounds %struct.CFI_STRUCT, %struct.CFI_STRUCT* %4, i32 0, i32 0
  store i64 %3, i64* %iCall, align 8
  %5 = load i64, i64* %call_target.addr, align 8
  %6 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %item, align 8
  %iTarget = getelementptr inbounds %struct.CFI_STRUCT, %struct.CFI_STRUCT* %6, i32 0, i32 1
  store i64 %5, i64* %iTarget, align 8
  %7 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %item, align 8
  %next = getelementptr inbounds %struct.CFI_STRUCT, %struct.CFI_STRUCT* %7, i32 0, i32 2
  store %struct.CFI_STRUCT* null, %struct.CFI_STRUCT** %next, align 8
  %8 = load i64, i64* %cfi_hash_key, align 8
  %arrayidx = getelementptr inbounds [1000000 x %struct.CFI_STRUCT*], [1000000 x %struct.CFI_STRUCT*]* @cfi_hash_table, i64 0, i64 %8
  %9 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %arrayidx, align 8
  %cmp = icmp eq %struct.CFI_STRUCT* %9, null
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %10 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %item, align 8
  %11 = load i64, i64* %cfi_hash_key, align 8
  %arrayidx1 = getelementptr inbounds [1000000 x %struct.CFI_STRUCT*], [1000000 x %struct.CFI_STRUCT*]* @cfi_hash_table, i64 0, i64 %11
  store %struct.CFI_STRUCT* %10, %struct.CFI_STRUCT** %arrayidx1, align 8
  br label %if.end

if.else:                                          ; preds = %entry
  %12 = load i64, i64* %cfi_hash_key, align 8
  %arrayidx2 = getelementptr inbounds [1000000 x %struct.CFI_STRUCT*], [1000000 x %struct.CFI_STRUCT*]* @cfi_hash_table, i64 0, i64 %12
  %13 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %arrayidx2, align 8
  store %struct.CFI_STRUCT* %13, %struct.CFI_STRUCT** %temp, align 8
  br label %while.cond

while.cond:                                       ; preds = %while.body, %if.else
  %14 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %temp, align 8
  %next3 = getelementptr inbounds %struct.CFI_STRUCT, %struct.CFI_STRUCT* %14, i32 0, i32 2
  %15 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %next3, align 8
  %cmp4 = icmp ne %struct.CFI_STRUCT* %15, null
  br i1 %cmp4, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %16 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %temp, align 8
  %next5 = getelementptr inbounds %struct.CFI_STRUCT, %struct.CFI_STRUCT* %16, i32 0, i32 2
  %17 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %next5, align 8
  store %struct.CFI_STRUCT* %17, %struct.CFI_STRUCT** %temp, align 8
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %18 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %item, align 8
  %19 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %temp, align 8
  %next6 = getelementptr inbounds %struct.CFI_STRUCT, %struct.CFI_STRUCT* %19, i32 0, i32 2
  store %struct.CFI_STRUCT* %18, %struct.CFI_STRUCT** %next6, align 8
  br label %if.end

if.end:                                           ; preds = %while.end, %if.then
  ret void
}

; Function Attrs: nounwind
declare dso_local noalias i8* @malloc(i64) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @cfi_free() #0 {
entry:
  %0 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8
  %1 = load i32, i32* @passCounter, align 4
  %call = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %0, i8* getelementptr inbounds ([19 x i8], [19 x i8]* @.str.1, i32 0, i32 0), i32 %1)
  ret void
}

declare dso_local i32 @fprintf(%struct._IO_FILE*, i8*, ...) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @cfi_hash_check(i64 %call_point, i64 %call_target) #0 {
entry:
  %retval = alloca i32, align 4
  %call_point.addr = alloca i64, align 8
  %call_target.addr = alloca i64, align 8
  %cfi_hash_key = alloca i64, align 8
  store i64 %call_point, i64* %call_point.addr, align 8
  store i64 %call_target, i64* %call_target.addr, align 8
  %0 = load i64, i64* %call_point.addr, align 8
  %1 = load i64, i64* %call_target.addr, align 8
  %xor = xor i64 %0, %1
  %rem = urem i64 %xor, 1000000
  store i64 %rem, i64* %cfi_hash_key, align 8
  %2 = load i64, i64* %cfi_hash_key, align 8
  %arrayidx = getelementptr inbounds [1000000 x %struct.CFI_STRUCT*], [1000000 x %struct.CFI_STRUCT*]* @cfi_hash_table, i64 0, i64 %2
  %3 = load %struct.CFI_STRUCT*, %struct.CFI_STRUCT** %arrayidx, align 8
  %cmp = icmp ne %struct.CFI_STRUCT* %3, null
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  store i32 1, i32* %retval, align 4
  br label %return

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval, align 4
  br label %return

return:                                           ; preds = %if.end, %if.then
  %4 = load i32, i32* %retval, align 4
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @pCall_reference_monitor(i64 %pCallID, i64 %target) #0 {
entry:
  %pCallID.addr = alloca i64, align 8
  %target.addr = alloca i64, align 8
  %pCall_point = alloca i64, align 8
  %pCall_target = alloca i64, align 8
  store i64 %pCallID, i64* %pCallID.addr, align 8
  store i64 %target, i64* %target.addr, align 8
  %0 = load i64, i64* %pCallID.addr, align 8
  store i64 %0, i64* %pCall_point, align 8
  %1 = load i64, i64* %target.addr, align 8
  store i64 %1, i64* %pCall_target, align 8
  %2 = load i64, i64* %pCall_point, align 8
  %3 = load i64, i64* %pCall_target, align 8
  %call = call i32 @cfi_hash_check(i64 %2, i64 %3)
  %tobool = icmp ne i32 %call, 0
  br i1 %tobool, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %4 = load i32, i32* @passCounter, align 4
  %inc = add i32 %4, 1
  store i32 %inc, i32* @passCounter, align 4
  br label %return

if.else:                                          ; preds = %entry
  %5 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8
  %6 = load i64, i64* %pCall_point, align 8
  %7 = load i64, i64* %pCall_target, align 8
  %call1 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %5, i8* getelementptr inbounds ([41 x i8], [41 x i8]* @.str, i32 0, i32 0), i64 %6, i64 %7)
  br label %return

return:                                           ; preds = %if.else, %if.then
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallDi(i32 %a) #0 {
entry:
  %a.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallFi(i32 %a) #0 {
entry:
  %a.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallEv() #0 {
entry:
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallAv() #0 {
entry:
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallBv() #0 {
entry:
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallCv() #0 {
entry:
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallGi(i32 %a) #0 {
entry:
  %a.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallHi(i32 %a) #0 {
entry:
  %a.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5CallIv() #0 {
entry:
  ret void
}

; Function Attrs: noinline norecurse optnone uwtable
define dso_local i32 @main() #3 personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
entry:
  %retval = alloca i32, align 4
  %h = alloca %class.Hello*, align 8
  %exn.slot = alloca i8*
  %ehselector.slot = alloca i32
  %lc = alloca %struct.ST, align 8
  store i32 0, i32* %retval, align 4
  %call = call i8* @_Znwm(i64 40) #7
  %0 = bitcast i8* %call to %class.Hello*
  invoke void @_ZN5HelloC2EPFvvE(%class.Hello* %0, void ()* @_Z5CallIv)
          to label %invoke.cont unwind label %lpad

invoke.cont:                                      ; preds = %entry
  store %class.Hello* %0, %class.Hello** %h, align 8
  %fp = getelementptr inbounds %struct.ST, %struct.ST* %lc, i32 0, i32 1
  store void (i32)* @_Z5CallGi, void (i32)** %fp, align 8
  %1 = load %class.Hello*, %class.Hello** %h, align 8
  %x = getelementptr inbounds %class.Hello, %class.Hello* %1, i32 0, i32 3
  store { i64, i64 } { i64 ptrtoint (void (%class.Hello*)* @_ZN5Hello5ptofnEv to i64), i64 0 }, { i64, i64 }* %x, align 8
  call void @_Z5CallBv()
  %fp1 = getelementptr inbounds %struct.ST, %struct.ST* %lc, i32 0, i32 1
  %2 = load void (i32)*, void (i32)** %fp1, align 8
  %3 = ptrtoint void (i32)* %2 to i64
  call void @pCall_reference_monitor(i64 133337585, i64 %3)
  call void %2(i32 10)
  %4 = load %class.Hello*, %class.Hello** %h, align 8
  %5 = bitcast %class.Hello* %4 to void (%class.Hello*)***
  %vtable = load void (%class.Hello*)**, void (%class.Hello*)*** %5, align 8
  %vfn = getelementptr inbounds void (%class.Hello*)*, void (%class.Hello*)** %vtable, i64 0
  %6 = load void (%class.Hello*)*, void (%class.Hello*)** %vfn, align 8
  %7 = ptrtoint void (%class.Hello*)* %6 to i64
  call void @pCall_reference_monitor(i64 53415718, i64 %7)
  call void %6(%class.Hello* %4)
  ret i32 0

lpad:                                             ; preds = %entry
  %8 = landingpad { i8*, i32 }
          cleanup
  %9 = extractvalue { i8*, i32 } %8, 0
  store i8* %9, i8** %exn.slot, align 8
  %10 = extractvalue { i8*, i32 } %8, 1
  store i32 %10, i32* %ehselector.slot, align 4
  call void @_ZdlPv(i8* %call) #8
  br label %eh.resume

eh.resume:                                        ; preds = %lpad
  %exn = load i8*, i8** %exn.slot, align 8
  %sel = load i32, i32* %ehselector.slot, align 4
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %exn, 0
  %lpad.val2 = insertvalue { i8*, i32 } %lpad.val, i32 %sel, 1
  resume { i8*, i32 } %lpad.val2
}

declare dso_local i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin
declare dso_local noalias i8* @_Znwm(i64) #4

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN5HelloC2EPFvvE(%class.Hello* %this, void ()* %f) unnamed_addr #0 comdat align 2 {
entry:
  %this.addr = alloca %class.Hello*, align 8
  %f.addr = alloca void ()*, align 8
  store %class.Hello* %this, %class.Hello** %this.addr, align 8
  store void ()* %f, void ()** %f.addr, align 8
  %this1 = load %class.Hello*, %class.Hello** %this.addr, align 8
  %0 = bitcast %class.Hello* %this1 to i32 (...)***
  store i32 (...)** bitcast (i8** getelementptr inbounds ({ [3 x i8*] }, { [3 x i8*] }* @_ZTV5Hello, i32 0, inrange i32 0, i32 2) to i32 (...)**), i32 (...)*** %0, align 8
  %fp1 = getelementptr inbounds %class.Hello, %class.Hello* %this1, i32 0, i32 1
  store void ()* @_Z5CallAv, void ()** %fp1, align 8
  %1 = load void ()*, void ()** %f.addr, align 8
  %fp2 = getelementptr inbounds %class.Hello, %class.Hello* %this1, i32 0, i32 2
  store void ()* %1, void ()** %fp2, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN5Hello5ptofnEv(%class.Hello* %this) #0 comdat align 2 {
entry:
  %this.addr = alloca %class.Hello*, align 8
  store %class.Hello* %this, %class.Hello** %this.addr, align 8
  %this1 = load %class.Hello*, %class.Hello** %this.addr, align 8
  ret void
}

; Function Attrs: nobuiltin nounwind
declare dso_local void @_ZdlPv(i8*) #5

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN5Hello5vFuncEv(%class.Hello* %this) unnamed_addr #0 comdat align 2 {
entry:
  %this.addr = alloca %class.Hello*, align 8
  store %class.Hello* %this, %class.Hello** %this.addr, align 8
  %this1 = load %class.Hello*, %class.Hello** %this.addr, align 8
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noinline norecurse optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nobuiltin "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nobuiltin nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { nounwind }
attributes #7 = { builtin }
attributes #8 = { builtin nounwind }

!llvm.ident = !{!0, !0}
!llvm.module.flags = !{!1}

!0 = !{!"clang version 9.0.0 (trunk 351861) (llvm/trunk 351852)"}
!1 = !{i32 1, !"wchar_size", i32 4}
