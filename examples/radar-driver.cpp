#include <ntddk.h>

#include <eastl_compat.hpp>
#include <EASTL/algorithm.h>
#include <EASTL/array.h>
#include <except.h>
#include <DriverThread.hpp>

#include <ipc.hpp>
#include <memory.hpp>

#include "common.hpp"

using namespace DriverThread;

static Thread thread;
static Event shutdown_event;

static PEPROCESS game_process = nullptr;
static HANDLE game_process_obj = nullptr;

static constexpr LONGLONG ONE_SECOND_RELATIVE = (-10LL * 1000LL * 1000LL);

extern "C" {
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

int shm_exception_handler(_In_ EXCEPTION_POINTERS *lpEP) {
  (void)lpEP;
  return EXCEPTION_EXECUTE_HANDLER;
}

static bool scan_game_memory(const Process& target,
                             PatternScanner::ResultVec& out) {
  auto pid = reinterpret_cast<HANDLE>(target.UniqueProcessId);
  const std::initializer_list<uint8_t> magic = {0xCA, 0xFE, 0xCA, 0xFE,
                                                0xBE, 0xEF, 0xBE, 0xEF};
  const eastl::string_view magic_mask = "xxxxxxxx";

  if (!NT_SUCCESS(::OpenProcess(pid, &game_process, &game_process_obj))) {
    game_process = nullptr;
    game_process_obj = nullptr;
    return false;
  }

  auto keep_process_open = false;
  SCOPE_EXIT([&]() {
    if (!keep_process_open) {
      ::CloseProcess(&game_process, &game_process_obj);
      game_process = nullptr;
      game_process_obj = nullptr;
    }
  });

  PatternScanner::Page scanner(game_process, game_process_obj, magic, magic_mask);
  auto found = scanner.Scan([](const Page & page) {
    return page.BaseAddress >= 0x00007FF000000000;
  }, out, 2);
  if (!found)
    return false;
  if (out.size() != 1)
    return false;

  keep_process_open = true;
  return true;
}

static auto find_user_game(const eastl::vector<Process>& procs) {
  const wchar_t target_process[] = L"fake-game.exe";
  const auto &found = eastl::find_if(
    procs.begin(), procs.end(), [&target_process](const auto &item) {
      return (item.ProcessName == target_process);
    }
  );
  if (found == procs.end())
    DbgPrint("Fake Game process not found: '%ws'\n", target_process);
  else
    DbgPrint("Process '%ws' pid: %zu\n", target_process,
             found->UniqueProcessId);
  return found;
}

static bool find_user_radar(const eastl::vector<Process>& procs,
                            IPC::KernelSharedMemory& kshm) {
  const wchar_t targetProcess[] = L"radar.exe";
  const auto &found = eastl::find_if(
    procs.begin(), procs.end(), [&targetProcess](const auto &item) {
      if (item.ProcessName == targetProcess)
        return true;
      return false;
  });
  if (found == procs.end()) {
    DbgPrint("Usermode IPC process not found: '%ws'\n", targetProcess);
    return false;
  }
  DbgPrint("Process '%ws' pid: %zu\n", targetProcess, found->UniqueProcessId);

  if (!kshm.FindSharedMemory({sizeof(radar_buffer_data), COMMON_BUFFER_SLOTS},
                             {sizeof(radar_ringbuffer_data), COMMON_RINGBUFFER_SLOTS}, *found))
  {
    DbgPrint("Shared Memory not found!\n");
    return false;
  }
  DbgPrint("IPC Memory chunks: %zu\n", kshm.AmountOfChunks());
  return true;
}

static void main_thread() {
  ExTimer timer;
  Event ipc;
  auto do_shutdown = false;

  timer.Start([&ipc]() {
    ipc.Notify();
  }, -166666, true, true);

  while (!do_shutdown &&
         shutdown_event.Wait(ONE_SECOND_RELATIVE) == STATUS_TIMEOUT)
  {
    IPC::KernelSharedMemory kshm;

    auto procs = ::GetProcesses();
    DbgPrint("Got %zu processes on this machine\n", procs.size());

    if (!find_user_radar(procs, kshm)) {
      DbgPrint("No user radar found, retry..\n");
      continue;
    }

    const auto& game_process_iter = find_user_game(procs);
    if (game_process_iter == procs.end()) {
      DbgPrint("No user game found, retry..\n");
      continue;
    }

    PatternScanner::ResultVec game_results;
    if (!scan_game_memory(*game_process_iter, game_results) || game_results.size() != 1) {
      DbgPrint("Fake Game memory scan failed, results: %zu\n", game_results.size());
      continue;
    }
    SCOPE_EXIT([]() {
      ::CloseProcess(&game_process, &game_process_obj);
      game_process = nullptr;
      game_process_obj = nullptr;
    });
    procs.clear();

    const auto g_entities = game_results[0];
    Memory memory(game_process);
    const eastl::function<uint64_t(void)> get_entities_ptr = [&g_entities, &memory]()
    {
      return memory.Read<uint64_t>(g_entities.BaseAddress + g_entities.Offset
                                   + 8 /* magic */
                                   + 4 /* max_entities */ + 4 /* amount_of_entities */);
    };
    const eastl::function<uint32_t(void)> get_entities_used = [&g_entities, &memory]()
    {
      return memory.Read<uint32_t>(g_entities.BaseAddress + g_entities.Offset
                                   + 8 /* magic */ + 4 /* max_entities */);
    };
    DbgPrint("Entity Ptr: 0x%llX, Amount of Entites: %u\n",
             (unsigned long long int)get_entities_ptr(), get_entities_used());
    DbgPrint("sizeof(radar_entity)=%zu sizeof(radar_buffer_data)=%zu\n",
             sizeof(radar_entity), sizeof(radar_buffer_data));

    const eastl::function<bool(uint64_t, uint64_t, radar_entity &)> get_entity =
    [&memory](uint64_t entity_ptr, uint64_t index, radar_entity & re)
    {
      return memory.ReadBuffer<radar_entity, 1>(entity_ptr + index * sizeof(radar_entity), &re);
    };

    kshm.WriteRingbufferData([](void * data) {
      auto slot_data = reinterpret_cast<struct radar_ringbuffer_data*>(data);
      strncpy(slot_data->msg, "Welcome.", strlen("Welcome."));
    });

    while (kshm.ProcessEvents(0LL) != false
           && ipc.WaitIndefinitely() == STATUS_SUCCESS)
    {
      if (shutdown_event.Wait(0LL) != STATUS_TIMEOUT) {
        do_shutdown = true;
        break;
      }
      ipc.Reset();

      const auto entites_ptr = get_entities_ptr();
      if (entites_ptr == 0)
        continue;
      const auto amount_of_entities = get_entities_used();
      radar_buffer_data radar_data = {};
      for (auto i = 0u; i < amount_of_entities && i < MAX_ENTITIES; ++i) {
        radar_entity radar_ent;
        if (!get_entity(entites_ptr, i, radar_ent))
          continue;

#if 0
        DbgPrint("[%u] addr=0x%llX name='%.*s'\n", i,
                 (unsigned long long)(entites_ptr + i * sizeof(radar_entity)),
                 (int)sizeof(radar_ent.name), radar_ent.name);
#endif
        radar_data.entities[radar_data.count] = radar_ent;
        radar_data.count++;
      }

      kshm.WriteBufferData([&radar_data](void *data) {
        auto slot_data = reinterpret_cast<struct radar_buffer_data*>(data);
        *slot_data = radar_data;
      });
    }
  }

  DbgPrint("Thread done.\n");
}

NTSTATUS DriverEntry(_In_ struct _DRIVER_OBJECT *DriverObject,
                     _In_ PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(DriverObject);
  UNREFERENCED_PARAMETER(RegistryPath);

  DbgPrint("%s\n", "Starting Main Thread..");
  auto args = eastl::make_shared<ThreadArgs>();
  thread.Start([](eastl::shared_ptr<ThreadArgs> args) {
    UNREFERENCED_PARAMETER(args);
    main_thread();
    return STATUS_SUCCESS;
  }, args);

  return STATUS_SUCCESS;
}

int unload_exception_handler(_In_ EXCEPTION_POINTERS *lpEP) {
  (void)lpEP;
  return EXCEPTION_EXECUTE_HANDLER;
}

VOID DriverUnload(_In_ struct _DRIVER_OBJECT *DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);

  __dpptry(unload_exception_handler, unload_seh) {
    shutdown_event.Notify();
    thread.WaitForTerminationIndefinitely();
  }
  __dppexcept(unload_seh) {}
  __dpptryend(unload_seh);
}
}
