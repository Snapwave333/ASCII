## IOCP Networking Upgrade
- Convert `TcpDirectorClient` to IOCP:
  - Create completion port, associate socket, use overlapped `WSARecv/WSASend`.
  - Replace non-blocking connect loop (`src/DirectorClientTcp.cpp:154-157`) with overlapped connect and completion handling.
  - Dispatch completions into the executor; one worker per completion batch.
- Endpoint handling:
  - Honor `NG_TEST_ENDPOINT` for real traffic; remove sleeps and simulated packets.
  - Token-bucket rate limiter per endpoint to shape traffic under load.

## Executor Enhancements
- Replace single central queue with per-worker deques and stealing for balanced scheduling.
- Micro-batching of small tasks to reduce contention and context switching.
- Thread affinity and optional NUMA pinning; expose simple config flags.
- Task tagging for end-to-end spans and percentile latency aggregation.

## Multi-Layer Caching Integration
- L1 per-thread TTL cache: lock-free open-address map; integrate in `Theme.cpp`, `AIDirector.cpp`, `ScenarioManager.cpp` hot paths.
- L2 segmented LRU (already added header): wire into request paths as shared cache for cross-thread hits.
- L3 Redis cache:
  - Vendor `redis-plus-plus` client and add connection pool with pipelining.
  - Write-through for hot invariants; strict consistency for idempotent reads.
- Invalidation:
  - Add pub/sub channel for vector-clock-like invalidations; bound staleness windows.

## Monitoring & Microsecond Tracing
- Extend `PerformanceMonitor::MonitoringLoop` (`src/production/PerformanceManagementSystem.cpp:153-171`) to ingest microsecond spans.
- Percentile computation utilities and per-component latency histograms.
- Stream metrics to resilience dashboard and logs with stable format.

## Load Testing Enhancements
- Expand orchestrator profiles to drive IOCP sockets and real HTTP endpoints at scale.
- Add validations: uniform completion dispersion, queue depth balance, p50/p99/p99.9 latency targets, error-rate bounds.
- Reporting: extend JSON reports with span percentiles, cache hit ratios and breaker events.

## Resilience & Failover
- Enhance `RedundancyManager` with hot-standby connections and instant cutover on breaker open.
- Integrate circuit breaker (`src/chaos/CircuitBreaker.cpp:510-566`) around I/O operations with jittered exponential backoff.
- Expose breaker metrics and failover state to `ResilienceDashboard`.

## Deliverables & Order
1. IOCP conversion of `TcpDirectorClient` with executor dispatch
2. Executor work-stealing deques, micro-batching, and affinity options
3. L1/L2/L3 caches in HTTP/A.I. paths with real data and invalidations
4. Monitoring span ingestion and percentile metrics
5. Orchestrator validations and enhanced reporting
6. Resilience: hot-standby failover and breaker telemetry

## Validation Criteria
- Real I/O only (no mocks); sustained overlapped operations with microsecond metrics.
- In-memory cache hits ≤ 5 μs p99; network cold path ≤ 50 ms p99.
- Linear throughput scaling to core count; balanced worker queues and IOCP completions.
- No critical errors during breaker transitions and failover.

Confirm to proceed with this implementation sequence; I will begin with IOCP conversion and executor upgrades, then integrate caches and monitoring, followed by load testing and resilience improvements.