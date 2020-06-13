#if !defined(CIRCULAR_DOUBLE_LINKED_LIST_H)
#define CIRCULAR_DOUBLE_LINKED_LIST_H

//T must include ->prev and ->next member
#define DECLARE_CIRCULAR_DOUBLE_LINKED_LIST(T)          \
  static inline T* list_ ## T ## _add_to(T* source, T* item) {  \
    T* last_next = source->next;                        \
    source->next = item;                                \
    item->prev = source;                                \
    item->next = last_next;                             \
    last_next->prev = item;                             \
    return source;                                      \
  }                                                     \
  static inline T*  list_ ## T ## _remove(T* item) {             \
    T* prev = item->prev;                               \
    item->prev->next = item->next;                      \
    item->next->prev = item->prev;                      \
    return prev == item ? NULL : prev;                  \
  }

#endif // CIRCULAR_DOUBLE_LINKED_LIST_H
