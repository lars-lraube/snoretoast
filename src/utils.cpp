/*
    SnoreToast is capable to invoke Windows 8 toast notifications.
    Copyright (C) 2019  Hannah von Reth <vonreth@kde.org>

    SnoreToast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SnoreToast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnoreToast.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "utils.h"
#include "snoretoasts.h"

#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>



using namespace Microsoft::WRL;

namespace {
    bool s_registered = false;
}
namespace Utils {

HRESULT registerActivator()
{
    if (!s_registered)
    {
        s_registered = true;
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::Create([] {});
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().IncrementObjectCount();
        return Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().RegisterObjects();
    }
    return S_OK;
}

void unregisterActivator()
{
    if (s_registered)
    {
        s_registered = false;
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().UnregisterObjects();
        Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule().DecrementObjectCount();
    }
}

std::unordered_map<std::wstring, std::wstring> splitData(const std::wstring &data)
{
    std::unordered_map<std::wstring, std::wstring> out;
    std::wstring tmp;
    std::wstringstream wss(data);
    while(std::getline(wss, tmp, L';'))
    {
        const auto pos = tmp.find(L"=");
        out[tmp.substr(0, pos)] = tmp.substr(pos + 1);
    }
    return out;
}

const std::wstring &selfLocate()
{
    static std::wstring path;
    if (path.empty())
    {
        size_t size;
        do {
            path.resize(path.size() + 1024);
            size = GetModuleFileNameW(nullptr, const_cast<wchar_t*>(path.data()), static_cast<DWORD>(path.size()));
        } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
        path.resize(size);
    }
    return path;
}

bool writePipe(const std::wstring &pipe, const std::wstring &data)
{
    HANDLE hPipe = CreateFile(pipe.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        const DWORD toWrite = static_cast<DWORD>(data.size() * sizeof(wchar_t));
        WriteFile(hPipe, data.c_str(), toWrite, &written, nullptr);
        const bool success = written == toWrite;
        tLog << (success ? L"Wrote: " : L"Failed to write: ") << data << " to " << pipe;
        WriteFile(hPipe, nullptr, sizeof(wchar_t), &written, nullptr);
        CloseHandle(hPipe);
        return success;
    }
	tLog << L"Failed to open pipe: " << pipe << L" data: " << data;
    return false;
}

std::wstring formatData(const std::vector<std::pair<std::wstring, std::wstring> > &data)
{
    std::wstringstream out;
    for (const auto &p : data)
    {
        if (!p.second.empty())
        {
            out << p.first << L"=" << p.second << L";";
        }
    }
    return out.str();
}

}

ToastLog::ToastLog()
{
    *this << Utils::selfLocate() <<  L" v" << SnoreToasts::version() << L"\n\t";
}

ToastLog::~ToastLog()
{
	m_log << L"\n";
    OutputDebugStringW(m_log.str().c_str());
}
