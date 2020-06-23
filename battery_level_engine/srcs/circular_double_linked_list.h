#if !defined(CIRCULAR_DOUBLE_LINKED_LIST_H)
#define CIRCULAR_DOUBLE_LINKED_LIST_H

//T must include ->prev and ->next member
#define DECLARE_NAMED_CIRCULAR_DOUBLE_LINKED_LIST(T, name)  \
  static inline T* name ## _add_after(T* source, T* item) { \
    T* last_next = source->next;                            \
    source->next = item;                                    \
    item->prev = source;                                    \
    item->next = last_next;                                 \
    last_next->prev = item;                                 \
    return source;                                          \
  }                                                         \
  static inline T* name ## _add_before(T* source, T* item) {\
    T* last_prev = source->prev;                            \
    source->prev = item;                                    \
    item->next = source;                                    \
    item->prev = last_prev;                                 \
    last_prev->next = item;                                 \
    return source;                                          \
  }                                                         \
  static inline T*  name ## _remove(T* item) {              \
    T* next = item->next;                                   \
    item->prev->next = item->next;                          \
    item->next->prev = item->prev;                          \
    return next == item ? NULL : next;                      \
  }

#define DECLARE_CIRCULAR_DOUBLE_LINKED_LIST(T) DECLARE_NAMED_CIRCULAR_DOUBLE_LINKED_LIST(T, list_ ## T)

#endif // CIRCULAR_DOUBLE_LINKED_LIST_H
