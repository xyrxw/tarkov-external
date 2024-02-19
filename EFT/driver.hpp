#define SYSCALL_CODE 0xDEADBEEF

#include "../HyperV/HyperV.h"




enum operation : int
{
    memory_read = 0,
    memory_write,
    module_base,
};

struct cmd_t
{
    bool success = false;
    unsigned int verification_code = 0;
    operation operation;
    void* buffer;
    ULONG64    address;
    ULONG size;
    ULONG pid;
    const char* module_name;
    ULONG64 base_address;
};

namespace driver
{

    int process_id;

    __int64(__fastcall* NtUserSetInteractiveCtrlRotationAngle)(void* a1) = nullptr;

    HyperV* hv = new HyperV();

    int get_process_id(const char* process_name)
    {
        PROCESSENTRY32 proc_info;
        proc_info.dwSize = sizeof(proc_info);

        const auto proc_snapshot =
            CreateToolhelp32Snapshot(
                TH32CS_SNAPPROCESS,
                NULL
            );

        if (proc_snapshot == INVALID_HANDLE_VALUE)
            return NULL;

        Process32First(proc_snapshot, &proc_info);
        if (!strcmp(proc_info.szExeFile, process_name)) {
            CloseHandle(proc_snapshot);
            return proc_info.th32ProcessID;
        }

        while (Process32Next(proc_snapshot, &proc_info)) {
            if (!strcmp(proc_info.szExeFile, process_name)) {
                CloseHandle(proc_snapshot);
                return proc_info.th32ProcessID;
            }
        }

        CloseHandle(proc_snapshot);
        return {};
    }

    bool setup()
    {
        //LoadLibraryA("user32.dll");
        //LoadLibraryA("win32u.dll");

        //const HMODULE win32u = GetModuleHandleA("win32u.dll");
        //if (!win32u)
        //    return false;

        //*(void**)&NtUserSetInteractiveCtrlRotationAngle = GetProcAddress(win32u, "NtUserSetInteractiveCtrlRotationAngle");

        //return NtUserSetInteractiveCtrlRotationAngle;
        auto pid = driver::get_process_id(e("EscapeFromTarkov.exe"));
        if (!hv->GetExtendProcCr3(pid))
            return false;
    }

    ULONG64 get_game_base()
    {
        return hv->GetProccessBase();
    }
    ULONG64 get_mono_module()
    {
        return hv->GetProcessModule(L"mono-2.0-bdwgc.dll");
    }
    ULONG64 get_unity_player()
    {
        return hv->GetProcessModule(L"UnityPlayer.dll");
    }




    template <typename type>
    inline type read(ULONG64 address)
    {
        return hv->ReadValue64<type>(address);
    }
    template <typename type>
    type read_array(ULONG64 address, type buffer, size_t size) {
        hv->ReadMem(address, &buffer, size);
        return buffer;
    }
    template <typename Type>
    static Type read_chain(uint64_t address, std::vector<uint64_t> chain) {
        uint64_t current = address;
        for (int i = 0; i < chain.size() - 1; i++) {
            current = read<uint64_t>(current + chain[i]);
        }
        return read<Type>(current + chain[chain.size() - 1]);
    }

    template <typename type>
    inline  void write(ULONG64 address, type value)
    {
        hv->WriteValue64<type>(address, value);
    }
    template <typename type>
    inline  void raw_write(ULONG64 address, type value, size_t size)
    {
        hv->WriteValue64<type>(address, value);
    }

    inline void readsize(ULONG64 address, void* buffer, size_t size) {
        hv->ReadMem((PVOID)address, buffer, size);
    }
    template<typename T>
    inline std::basic_string<T> ReadString(std::uintptr_t address) {

        std::basic_string<T> result;

        using StringType = std::basic_string<T>;
        using ValueType = typename StringType::value_type;

        std::size_t count{ sizeof(ValueType) * 25 };
        bool done{ false };

        for (std::uintptr_t it{ address }; !done; it += count) {

            const auto buffer = std::make_unique<ValueType[]>(count);

            if (!readsize(it, buffer.get(), count)) {
                return result;
            }

            for (std::size_t i{ 0 }; i < count; i++) {

                if (!buffer.get()[i]) {

                    done = true;
                    break;
                }

                result.push_back(buffer.get()[i]);
            }
        }

        return result;
    }

}
