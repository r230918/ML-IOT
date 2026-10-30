#include <stdint.h>
#include <errno.h>

extern uint32_t end;
extern uint32_t _estack;

void *_sbrk(ptrdiff_t incr) {
  static uint8_t *__sbrk_heap_end = NULL;
  uint8_t *prev_heap_end;

  if (__sbrk_heap_end == NULL) {
    __sbrk_heap_end = (uint8_t *)&end;
  }

  prev_heap_end = __sbrk_heap_end;
  if (__sbrk_heap_end + incr > (uint8_t *)&_estack) {
    errno = ENOMEM;
    return (void *)-1;
  }

  __sbrk_heap_end += incr;
  return (void *)prev_heap_end;
}
