// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#include <memory>
#include <mutex>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/message_loop/message_pump_type.h"
#include "base/notimplemented.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread.h"
#include "net/base/network_change_notifier.h"
#include "net/base/network_anonymization_key.h"
#include "net/proxy_resolution/configured_proxy_resolution_service.h"
#include "net/proxy_resolution/proxy_config_service.h"

// This file provides minimal "stub" implementations of the Cronet global-state
// functions for the native library build, sufficient to have cronet_tests and
// cronet_unittests build.

namespace cronet {

namespace {

// Dedicated init thread - similar to Android's init thread
std::unique_ptr<base::Thread> g_init_thread;
scoped_refptr<base::SingleThreadTaskRunner> g_init_task_runner;
std::unique_ptr<net::NetworkChangeNotifier> g_network_change_notifier;

// Signaled when init thread initialization is complete
base::WaitableEvent* g_init_done = nullptr;

std::once_flag g_init_flag;

void InitializeOnInitThread() {
  // Initialize NetworkChangeNotifier on the init thread
  // This is required for network operations
  g_network_change_notifier = net::NetworkChangeNotifier::CreateIfNeeded();
  g_init_done->Signal();
}

void DoInitialize() {
// Cronet tests sets AtExitManager as part of TestSuite, so statically linked
// library is not allowed to set its own.
#if !defined(CRONET_TESTS_IMPLEMENTATION)
  // Intentionally leak AtExitManager - it must outlive everything
  new base::AtExitManager;
#endif

  // Initialize CommandLine - required by many Chromium components
  base::CommandLine::Init(0, nullptr);

  // Initialize logging
  logging::InitLogging(logging::LoggingSettings());

  base::FeatureList::InitInstance(std::string(), std::string());

  // Enable NAK partitioning to allow separate connection pools per NAK.
  // This is required for insecure_concurrency feature.
  net::NetworkAnonymizationKey::PartitionByDefault();

  // Note that in component builds this ThreadPoolInstance will be shared with
  // the calling process, if it also depends on //base. In particular this means
  // that the Cronet test binaries must avoid initializing or shutting-down the
  // ThreadPoolInstance themselves.
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("cronet");

  // Create a dedicated init thread with its own message loop
  // This is similar to how Android creates g_init_task_executor
  g_init_thread = std::make_unique<base::Thread>("CronetInit");
  base::Thread::Options options;
  options.message_pump_type = base::MessagePumpType::IO;
  g_init_thread->StartWithOptions(std::move(options));
  g_init_task_runner = g_init_thread->task_runner();

  // Create waitable event for synchronization
  g_init_done = new base::WaitableEvent(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);

  // Complete initialization on the init thread
  g_init_task_runner->PostTask(FROM_HERE, base::BindOnce(&InitializeOnInitThread));

  // Wait for init thread initialization to complete
  g_init_done->Wait();
}

}  // namespace

void EnsureInitialized() {
  std::call_once(g_init_flag, &DoInitialize);
}

bool OnInitThread() {
  // Must call EnsureInitialized first to avoid null pointer
  EnsureInitialized();
  return g_init_task_runner->BelongsToCurrentThread();
}

void PostTaskToInitThread(const base::Location& posted_from,
                          base::OnceClosure task) {
  EnsureInitialized();
  g_init_task_runner->PostTask(posted_from, std::move(task));
}

std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner) {
  return net::ProxyConfigService::CreateSystemProxyConfigService(
      io_task_runner);
}

std::unique_ptr<net::ProxyResolutionService> CreateProxyResolutionService(
    std::unique_ptr<net::ProxyConfigService> proxy_config_service,
    net::NetLog* net_log) {
  return net::ConfiguredProxyResolutionService::CreateUsingSystemProxyResolver(
      std::move(proxy_config_service), net_log, /*quick_check_enabled=*/true);
}

std::string CreateDefaultUserAgent(const std::string& partial_user_agent) {
  // Do NOT call EnsureInitialized() here!
  // This function is called from SharedEngineState constructor (via base::NoDestructor),
  // which happens during static initialization, before AtExitManager is created.
  // Calling EnsureInitialized() here would cause base::Lock to fail.
  return partial_user_agent;
}

void SetNetworkThreadPriorityOnNetworkThread(double priority) {
  NOTIMPLEMENTED();
}

}  // namespace cronet
