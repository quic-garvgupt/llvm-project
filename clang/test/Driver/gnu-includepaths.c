// Verify that Gnu (Generic_ELF) driver forwards user -I as --includepaths to ld.lld,
// irrespective of LTO enablement, preserves ordering for multiple -I, emits nothing
// when no -I is provided, and does not forward for non-lld linkers.
//
// RUN: rm -rf %t && mkdir -p %t && mkdir -p %t/inc %t/dir1 %t/dir2 %t/'dir with space'
//
// With LTO enabled, -I is forwarded.
// RUN: %clang -### --target=riscv32-unknown-linux-gnu -nostdlib -fuse-ld=lld -flto %s -I %t/inc 2>&1 | FileCheck %s --check-prefix=WITHLTO
//
// Without LTO, -I is still forwarded.
// RUN: %clang -### --target=riscv32-unknown-linux-gnu -nostdlib -fuse-ld=lld %s -I %t/inc 2>&1 | FileCheck %s --check-prefix=NOLTO
//
// With no -I, nothing is forwarded.
// RUN: %clang -### --target=riscv32-unknown-linux-gnu -nostdlib -fuse-ld=lld %s 2>&1 | FileCheck %s --check-prefix=NOI
//
// Multiple -I are forwarded in order.
// RUN: %clang -### --target=riscv32-unknown-linux-gnu -nostdlib -fuse-ld=lld %s -I %t/dir1 -I %t/dir2 2>&1 | FileCheck %s --check-prefix=MULTI
//
// Non-LLD linker does not receive includepaths.
// RUN: %clang -### --target=riscv32-unknown-linux-gnu -nostdlib -fuse-ld=gold %s -I %t/inc 2>&1 | FileCheck %s --check-prefix=NONLLD
//
// Path with spaces is forwarded (argument may be quoted by the driver).
// RUN: %clang -### --target=riscv32-unknown-linux-gnu -nostdlib -fuse-ld=lld %s -I %t/'dir with space' 2>&1 | FileCheck %s --check-prefix=SPACE
//
// WITHLTO: "--includepaths=
// WITHLTO-SAME: {{.*}}inc
//
// NOLTO: "--includepaths=
// NOLTO-SAME: {{.*}}inc
//
// NOI-NOT: --includepaths
//
// MULTI: "--includepaths={{.*}}dir1"
// MULTI-SAME: "--includepaths={{.*}}dir2"
//
// NONLLD-NOT: --includepaths
//
// SPACE: --includepaths={{.*}}dir with space

int main(void) { return 0; }
