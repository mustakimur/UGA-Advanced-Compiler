#define CFI_HASH 1000000
#define MAX_ITEMS 10

__attribute__((__used__)) __attribute__((
    section("cfg_label_data"))) extern const int* CFG_TABLE[];

extern void vCall_reference_monitor(void *, unsigned long, void *);
extern void pCall_reference_monitor(unsigned long, unsigned long);

void cfi_init();
void cfi_free();