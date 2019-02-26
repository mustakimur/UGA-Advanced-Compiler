/*
 * Complete Control Flow Intergrity
 * Author: Mustakimur Rahman Khandaker
 */

#include <stdio.h>
#include <stdlib.h>

__attribute__((__used__)) unsigned int CFG_LENGTH = 0;

unsigned int passCounter = 0;

struct CFI_STRUCT {
  unsigned long iCall;
  unsigned long iTarget;
  struct CFI_STRUCT *next;
};
struct CFI_STRUCT *cfi_hash_table[CFI_HASH] = {NULL};

void cfi_hash_insert(unsigned long call_point, unsigned long call_target) {
  unsigned long cfi_hash_key = (call_point ^ call_target) % CFI_HASH;
  struct CFI_STRUCT *item =
      (struct CFI_STRUCT *)malloc(sizeof(struct CFI_STRUCT));
  item->iCall = call_point;
  item->iTarget = call_target;
  item->next = NULL;

  if (cfi_hash_table[cfi_hash_key] == NULL) {
    cfi_hash_table[cfi_hash_key] = item;
  } else {
    struct CFI_STRUCT *temp = cfi_hash_table[cfi_hash_key];
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = item;
  }
}

int cfi_hash_check(unsigned long call_point, unsigned long call_target) {
  unsigned long cfi_hash_key = (call_point ^ call_target) % CFI_HASH;
  if (cfi_hash_table[cfi_hash_key] != NULL) {
    return 1;
  }
  return 0;
}

void pCall_reference_monitor(unsigned long pCallID, unsigned long target) {
  unsigned long pCall_point = pCallID;
  unsigned long pCall_target = target;

  if (cfi_hash_check(pCall_point, pCall_target)) {
    passCounter++;
    return;
  } else {
    fprintf(stderr, "[pCall-CFI] Error at %ld target to 0x%x\n", pCall_point,
            pCall_target);
    return;
  }
}

void cfi_init() {
  unsigned long i;

  for (i = 0; i < CFG_LENGTH; i += 2) {
    cfi_hash_insert(CFG_TABLE[i], CFG_TABLE[i + 1]);
  }
}

void cfi_free() { fprintf(stderr, "Pass Counter: %ld\n", passCounter); }

__attribute__((section(".preinit_array"),
               used)) void (*_cfi_preinit)(void) = cfi_init;

__attribute__((section(".fini_array"),
               used)) void (*_cfi_fini)(void) = cfi_free;