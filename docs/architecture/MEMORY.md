# Memory architecture

## Regions
Internal SRAM for critical stacks, kernel state and latency-sensitive allocations; external PSRAM for larger general allocations when initialized and verified. DMA-capable memory must be explicitly selected by capability, not assumed to include all PSRAM.

## Interfaces (design only)
`maia_mem_region_register`, `maia_alloc`, `maia_alloc_caps`, `maia_free`, `maia_mem_stats`. All require alignment and overflow checks, lock/ISR contracts, and allocation-failure reporting.

## Constraints
Native tasks share a physical address space without a conventional MMU. No unverified memory protection promises. PSRAM/cache coherency and DMA ownership must be validated against ESP32-P4 documentation and board measurements. Memory usage budgets belong to applications; 16+16 MiB is a proposed MaiaOS configuration, not a kernel invariant.

## Tests
Fragmentation, alignment, invalid free, exhaustion, cross-core allocation, PSRAM initialization failure, DMA capability, cache coherence, and deterministic behavior after allocation failure.
