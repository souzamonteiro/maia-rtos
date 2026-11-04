# Public API proposal (not implemented)

Use opaque types: `maia_task_t`, `maia_mutex_t`, `maia_queue_t`. Define `maia_status_t` error enumeration. Proposed namespaces: `maia_task_*`, `maia_mutex_*`, `maia_sem_*`, `maia_queue_*`, `maia_timer_*`, `maia_mem_*`, `maia_device_*`.

Every API must document: parameter ownership, valid execution context (thread/ISR), blocking semantics, timeout units, core affinity, error values, resource lifetime and thread safety. Do not expose stub declarations as implemented interfaces. Version public API independently of kernel implementation.

Applications in C++ should include public headers through `extern "C"` guards. Avoid C++ exceptions across ABI boundaries.
