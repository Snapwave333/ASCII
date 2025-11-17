## IOCP Send Path
- Implement overlapped `WSASend` with per-connection send buffers and completion tracking.
- Integrate token-bucket rate limiter per endpoint; expose send queue depth metrics.
- Route send completions through the executor, batching small messages for fewer system calls.

## Executor Enhancements
- Replace central queue with per-worker deques and work-stealing.
- Add micro-batching and optional thread affinity/NUMA pinning flags.
- Tag tasks for end-to-end spans to compute percentile latency.

## Caching Integration (L1/L3)
- L1 per-thread TTL cache: open-address map for hot request paths in `AIDirector.cpp` and `ScenarioManager.cpp`.
- L3 Redis: vendor a C++ client (e.g., redis-plus-plus), add connection pool and pipelining; enforce write-through for hot invariants.
- Invalidation via pub/sub channel and bounded staleness windows.

## Monitoring & Percentiles
- Extend `PerformanceMonitor::MonitoringLoop` to ingest microsecond spans and compute p50/p90/p99/p99.9.
- Append per-component histograms; export stable metrics format to logs and Resilience Dashboard.

## Orchestrator Validations & Reports
- Validate worker queue depth balance and IOCP completion dispersion under extreme load.
- Add cache hit ratios, breaker events, and percentile latency summaries to JSON reports.

## Resilience & Failover
- Add hot-standby connections to enable instant cutover on breaker open.
- Wrap I/O ops with `ResilientCircuitBreaker::ExecuteWithRetry` and jittered exponential backoff.
- Expose breaker metrics to Resilience Dashboard for real-time health.

## Acceptance Criteria
- Real overlapped send/recv only (no mocks); sustained microsecond metrics recorded.
- In-memory cache hits ≤ 5 μs p99; network cold path ≤ 50 ms p99.
- Linear throughput scaling to core count with balanced work queues.
- No critical errors during breaker transitions and failover.

On approval, I will implement IOCP send, upgrade the executor, then integrate L1/L3 caches and monitoring, followed by validations and resilience improvements.