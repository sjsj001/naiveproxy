# NaiveProxy Cronet Fork

This fork of [NaiveProxy](https://github.com/klzgrad/naiveproxy) adds cronet static library support for [cronet-go](https://github.com/sagernet/cronet-go).

Based on NaiveProxy v140 (Chromium 140.0.7339.123).

## Changes from naiveproxy upstream

### Build Configuration Changes

| File | Change | Reason |
|------|--------|--------|
| `src/BUILD.gn` | Add `//components/cronet:cronet_static` | Build cronet static library |
| `src/build/config/cronet/config.gni` | `is_cronet_build = true` | Enable cronet build mode, avoid mojo dependency |
| `src/net/features.gni` | Disable `enable_mdns` | Avoid re2 dependency |
| `src/net/features.gni` | Disable `include_transport_security_state_preload_list` | Preload list JSON not included |
| `src/net/features.gni` | Disable `enable_device_bound_sessions` | Avoid sqlite_proto dependency |
| `src/net/features.gni` | Disable `enable_disk_cache_sql_backend` | Avoid sql dependency |
| `src/url/features.gni` | `use_platform_icu_alternatives = true` | Avoid ICU dependency |
| `src/third_party/perfetto/gn/perfetto.gni` | `perfetto_build_with_embedder = true` | Use Chromium paths |

### Bug Fixes

| File | Change | Reason |
|------|--------|--------|
| `src/net/socket/client_socket_pool_manager_impl.cc` | Set `is_for_websockets=false` | Fix HTTP/2 ALPN negotiation for BidirectionalStream |

### Added Components (from Chromium v140)

| Directory | Purpose |
|-----------|---------|
| `src/components/cronet/` | Cronet core implementation |
| `src/components/grpc_support/` | BidirectionalStream C API |
| `src/components/prefs/` | Preferences support |

### Custom API Extensions

| File | API | Purpose |
|------|-----|---------|
| `src/components/cronet/native/include/cronet_c.h` | `Cronet_CreateCertVerifierWithRootCerts()` | Custom root certificate validation |
| `src/components/cronet/native/include/cronet_c.h` | `Cronet_CreateCertVerifierWithPublicKeySHA256()` | Certificate pinning by public key SHA256 hash |
| `src/components/cronet/native/engine.cc` | Implementation | Support custom CA certificates and public key pinning |
| `src/components/grpc_support/include/bidirectional_stream_c.h` | `bidirectional_stream_set_concurrency_index()` | HTTP/2 connection pool isolation for insecure concurrency |

### Insecure Concurrency Support

Added support for HTTP/2 connection pool isolation using `NetworkAnonymizationKey` (NAK). This allows multiple independent HTTP/2 sessions to be created to the same server by assigning different concurrency indices.

| File | Change |
|------|--------|
| `src/components/grpc_support/bidirectional_stream_c.cc` | Global NAK cache with per-index transient NAK creation |
| `src/components/grpc_support/bidirectional_stream.h/cc` | NAK member and setter for BidirectionalStream |
| `src/net/http/bidirectional_stream_request_info.h` | NAK field in request info |
| `src/net/http/bidirectional_stream.cc` | Pass NAK to HttpStreamFactory, disable IP-based pooling for transient NAK |
| `src/components/cronet/cronet_global_state_stubs.cc` | Enable NAK partitioning via `PartitionByDefault()` |
| `src/components/cronet/android/cronet_library_loader.cc` | Enable NAK partitioning for Android |

### Custom Experimental Options

| Option | Parameters | Purpose |
|--------|------------|---------|
| `DnsServerOverride` | `nameservers`: list of `"ip:port"` | Override DNS nameservers for the built-in async DNS client |

Example:
```json
{
  "AsyncDNS": {"enable": true},
  "DnsServerOverride": {"nameservers": ["127.0.0.1:5353"]}
}
```

This allows redirecting DNS queries to a local DNS forwarder for custom DNS routing.
