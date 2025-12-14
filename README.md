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

### Android Stub Files

For Android builds without JNI (native-only Go binding), stub implementations replace JNI-dependent code:

| File | Replaces | Purpose |
|------|----------|---------|
| `src/base/android/path_utils_stub.cc` | `path_utils.cc` | Returns empty paths (cache/native lib dirs not needed) |
| `src/base/android/input_hint_checker_stub.cc` | `input_hint_checker.cc` | No-op (UI input detection not needed for network) |
| `src/base/android/android_info_stub.cc` | `android_info.cc` | Uses `__system_property_get` instead of JNI |
| `src/base/android/java_runtime_stub.cc` | `java_runtime.cc` | Returns 0 for Java heap memory (no JVM) |
| `src/components/prefs/android/pref_service_android_stub.cc` | `pref_service_android.cc` | No-op (Java preference bindings not needed) |
| `src/net/android/network_change_notifier_android_stub.cc` | `network_change_notifier_android.cc` | Provides `NetworkChangeCalculatorParamsAndroid()` only |
| `src/net/proxy_resolution/proxy_config_service_android_stub.cc` | `proxy_config_service_android.cc` | Returns direct (no proxy) configuration |

These stubs resolve linker errors when building cronet for Android without the full Chromium Java/JNI infrastructure.
