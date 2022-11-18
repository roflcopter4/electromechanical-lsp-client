#include "Common.hh"

#include "ipc/ipc_connection.hh"

#include "ipc/rpc/lsp.hh"
#include "ipc/rpc/lsp/static-data.hh"
#include "ipc/rpc/neovim.hh"

#include "lazy.hh"

#include <ipc/rpc/rpc.hh>

#include <tchar.h>

#define AUTOC auto const

inline namespace emlsp {
namespace testing {
/****************************************************************************************/


template <typename T>
using protocol_type   = ipc::protocols::Msgpack::connection<T>;
using connection_type = ipc::connections::unix_socket;


NOINLINE void foo20()
{
      using rpc_type = ipc::rpc::basic_rpc_connection<connection_type, protocol_type>;

      auto foo = rpc_type::new_shared();
}


// ReSharper disable CppJoinDeclarationAndAssignment

namespace {
//constexpr wchar_t test_file[] = LR"(D:\Downloads\things\11_Sorted\Games\@ARCHIVES\Corruption_of_Champions_II-0.5.11-win\resources\app\idiot.js)";
constexpr wchar_t test_file[] = LR"(D:\Downloads\things\11_Sorted\Games\@ARCHIVES\Corruption_of_Champions_II-0.5.11-win\resources\app\index.html)";

constexpr wchar_t lpcTheFile[] = LR"(alovelytestfile)";
constexpr size_t  FILE_MAP_START = 138240;
constexpr size_t  BUFFSIZE       = 1024;
}

NOINLINE int foo100()
{
      SYSTEM_INFO SysInfo;   // system information; used to get granularity
      HANDLE hMapFile;       // handle for the file's memory-mapped region
      HANDLE hFile;          // the file handle
      BOOL   bFlag;          // a result holder
      DWORD  dBytesWritten;  // number of bytes written
      DWORD  dwFileSize;     // temporary storage for file sizes
      DWORD  dwFileMapSize;  // size of the file mapping
      DWORD  dwMapViewSize;  // the size of the view
      DWORD  dwFileMapStart; // where to start the file map view
      DWORD  dwSysGran;      // system allocation granularity
      LPVOID lpMapAddress;   // pointer to the base address of the memory-mapped region
      char  *pData;          // pointer to the data
      int    iData;          // on success contains the first int of data
      size_t iViewDelta;     // the offset into the view where the data shows up

      // Create the test file. Open it "Create Always" to overwrite any
      // existing file. The data is re-created below
      hFile = ::CreateFileW(lpcTheFile, GENERIC_READ | GENERIC_WRITE,
                            0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

      if (hFile == INVALID_HANDLE_VALUE) {
            wprintf(L"hFile is NULL\nTarget file is %s\n", lpcTheFile);
            return 4;
      }

      // Get the system allocation granularity.
      ::GetSystemInfo(&SysInfo);
      dwSysGran = SysInfo.dwAllocationGranularity;

      // Now calculate a few variables. Calculate the file offsets as
      // 64-bit values, and then get the low-order 32 bits for the
      // function calls.

      // To calculate where to start the file mapping, round down the
      // offset of the data into the file to the nearest multiple of the
      // system allocation granularity.
      dwFileMapStart = (FILE_MAP_START / dwSysGran) * dwSysGran;
      wprintf(L"The file map view starts at %lu bytes into the file.\n", dwFileMapStart);

      // Calculate the size of the file mapping view.
      dwMapViewSize = (FILE_MAP_START % dwSysGran) + BUFFSIZE;
      wprintf(L"The file map view is %lu bytes large.\n", dwMapViewSize);

      // How large will the file mapping object be?
      dwFileMapSize = FILE_MAP_START + BUFFSIZE;
      wprintf(L"The file mapping object is %lu bytes large.\n", dwFileMapSize);

      // The data of interest isn't at the beginning of the
      // view, so determine how far into the view to set the pointer.
      iViewDelta = FILE_MAP_START - dwFileMapStart;
      wprintf(L"The data is %zu bytes into the view.\n", iViewDelta);

      // Now write a file with data suitable for experimentation. This
      // provides unique int (4-byte) offsets in the file for easy visual
      // inspection. Note that this code does not check for storage
      // medium overflow or other errors, which production code should
      // do. Because an int is 4 bytes, the value at the pointer to the
      // data should be one quarter of the desired offset into the file

      for (DWORD i = 0; i < dwSysGran; i++)
            ::WriteFile(hFile, &i, sizeof(i), &dBytesWritten, nullptr);

      // Verify that the correct file size was written.
      dwFileSize = ::GetFileSize(hFile, nullptr);
      wprintf(L"hFile size: %10lu\n", dwFileSize);

      // Create a file mapping object for the file
      // Note that it is a good idea to ensure the file size is not zero
      hMapFile = ::CreateFileMappingW(hFile,          // current file handle
                                      nullptr,        // default security
                                      PAGE_READWRITE, // read/write permission
                                      0,              // size of mapping object, high
                                      dwFileMapSize,  // size of mapping object, low
                                      nullptr);       // name of mapping object

      if (hMapFile == nullptr) {
            wprintf(L"hMapFile is NULL: last error: %lu\n", ::GetLastError());
            return (2);
      }

      // Map the view and test the results.

      lpMapAddress = ::MapViewOfFile(hMapFile,            // handle to mapping object
                                     FILE_MAP_ALL_ACCESS, // read/write
                                     0,                   // high-order 32 bits of file offset
                                     dwFileMapStart,      // low-order 32 bits of file offset
                                     dwMapViewSize);      // number of bytes to map
      if (lpMapAddress == nullptr) {
            wprintf(L"lpMapAddress is nullptr: last error: %lu\n", ::GetLastError());
            return 3;
      }

      // Calculate the pointer to the data.
      pData = static_cast<char *>(lpMapAddress) + iViewDelta;

      // Extract the data, an int. Cast the pointer pData from a "pointer
      // to char" to a "pointer to int" to get the whole thing
      iData = *reinterpret_cast<int *>(pData);

      wprintf(L"The value at the pointer is %d,\nwhich %s one quarter of the desired file offset.\n",
              iData,
              static_cast<size_t>(iData) * 4ULL == FILE_MAP_START ? L"is" : L"is not");

      // Close the file mapping object and the open file
      bFlag = ::UnmapViewOfFile(lpMapAddress);
      assert(bFlag);
      bFlag = ::CloseHandle(hMapFile); // close the file mapping object

      if (!bFlag)
            wprintf(L"\nError %lu occurred closing the mapping object!", ::GetLastError());

      bFlag = ::CloseHandle(hFile); // close the file itself

      if (!bFlag)
            wprintf(L"\nError %lu occurred closing the file!", ::GetLastError());

      return 0;
}


namespace win32 {

class large_integer
{
      ::LARGE_INTEGER val_;

    public:
      //operator ::LARGE_INTEGER &() & { return val_; }
      //operator ::LARGE_INTEGER *()   { return &val_; }
      //operator signed   long long() const { return static_cast<signed long long>(val_.QuadPart); }
      //operator signed   long()      const { return static_cast<signed long>(val_.QuadPart); }
      //operator signed   int()       const { return static_cast<signed int>(val_.QuadPart); }
      //operator unsigned long long() const { return static_cast<unsigned long long>(val_.QuadPart); }
      //operator unsigned long()      const { return static_cast<unsigned long>(val_.QuadPart); }
      //operator unsigned int()       const { return static_cast<unsigned>(val_.QuadPart); }

      template <typename T> requires util::concepts::Integral<T>
      operator T() const { return static_cast<T>(val_.QuadPart); }

      large_integer &operator=(uint64_t const value) noexcept
      {
            val_.QuadPart = static_cast<::LONGLONG>(value);
            return *this;
      }

      large_integer() : val_({.QuadPart = 0})
      {}
      explicit large_integer(uint64_t const value)
          : val_({.QuadPart = static_cast<::LONGLONG>(value)})
      {}
};

} // namespace win32


NOINLINE void foo101()
{
      HANDLE        hFile;
      HANDLE        hMapFile;
      LPVOID        lpMapAddress;
      LARGE_INTEGER fileSize;

      hFile = ::CreateFileW(test_file, GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
      if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"CreateFileW");

      hMapFile = ::CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
      if (hMapFile == nullptr || hMapFile == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"CreateFileMapping()");

      lpMapAddress = ::MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
      if (lpMapAddress == nullptr || lpMapAddress == INVALID_HANDLE_VALUE)
            util::win32::error_exit(L"MapViewOfFile()");

      ::GetFileSizeEx(hFile, &fileSize);

      fwrite(lpMapAddress, 1, fileSize.QuadPart, stderr);
      fputs("\n\n", stderr);

      auto const addr = reinterpret_cast<uintptr_t>(lpMapAddress);
      util::eprint(
          FC(L"Mapped file \"{}\" into memory at address {:04X}_{:04X}_{:04X}_{:04X}, size {:L} bytes.\n"),
          test_file,
          uint16_t((addr & UINT64_C(0xFFFF'0000'0000'0000)) >> 48),
          uint16_t((addr & UINT64_C(0x0000'FFFF'0000'0000)) >> 32),
          uint16_t((addr & UINT64_C(0x0000'0000'FFFF'0000)) >> 16),
          uint16_t((addr & UINT64_C(0x0000'0000'0000'FFFF)) >>  0),
          static_cast<uint64_t>(fileSize.QuadPart)
      );

      ::UnmapViewOfFile(lpMapAddress);
      ::CloseHandle(hMapFile);
      ::CloseHandle(hFile);
}

// ReSharper restore CppJoinDeclarationAndAssignment


/****************************************************************************************/
} // namespace testing
} // namespace emlsp
