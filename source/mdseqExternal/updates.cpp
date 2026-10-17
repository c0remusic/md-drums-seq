#include "updates.h"

#include "mdseqUpdates.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

namespace mdseqUpdates
{
	namespace
	{
		std::filesystem::path settingsPath()
		{
			const char* base = std::getenv("LOCALAPPDATA");
			std::error_code error;
			const std::filesystem::path folder = base && *base ? std::filesystem::path(base) / "md-drums-seq"
				: std::filesystem::temp_directory_path(error) / "md-drums-seq";
			return folder / "settings.txt";
		}

#ifdef _WIN32
		std::wstring wide(const char* _text)
		{
			return std::wstring(_text, _text + std::strlen(_text));
		}
#endif
	}

	Settings readSettings()
	{
		Settings s;
		std::ifstream file(settingsPath());
		std::string key;
		std::string value;
		while(file >> key)
		{
			std::getline(file >> std::ws, value);
			if(key == "updates")
				s.enabled = value != "off";
			else if(key == "checked")
				s.lastCheck = value;
			else if(key == "latest")
				s.latest = value;
		}
		return s;
	}

	void writeSettings(const Settings& _settings)
	{
		const std::filesystem::path path = settingsPath();
		std::error_code error;
		std::filesystem::create_directories(path.parent_path(), error);
		std::ofstream file(path, std::ios::trunc);
		file << "updates " << (_settings.enabled ? "on" : "off") << "\n";
		if(!_settings.lastCheck.empty())
			file << "checked " << _settings.lastCheck << "\n";
		if(!_settings.latest.empty())
			file << "latest " << _settings.latest << "\n";
	}

	std::string today()
	{
		const std::time_t now = std::time(nullptr);
		std::tm local{};
#ifdef _WIN32
		localtime_s(&local, &now);
#else
		localtime_r(&now, &local);
#endif
		char text[16];
		std::strftime(text, sizeof(text), "%Y-%m-%d", &local);
		return text;
	}

	std::optional<std::string> fetchReleases()
	{
#ifdef _WIN32
		std::optional<std::string> body;
		HINTERNET session = WinHttpOpen(L"md-drums-seq", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS, 0);
		if(!session)
			return body;
		WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
		HINTERNET connection = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
		HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", wide(mdseq::ReleasesApiPath).c_str(),
			nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
		if(request && WinHttpSendRequest(request, L"Accept: application/vnd.github+json\r\n", static_cast<DWORD>(-1),
			WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr))
		{
			DWORD status = 0;
			DWORD size = sizeof(status);
			WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
				&status, &size, WINHTTP_NO_HEADER_INDEX);
			if(status == 200)
			{
				std::string text;
				DWORD available = 0;
				bool complete = true;
				while(WinHttpQueryDataAvailable(request, &available) && available > 0)
				{
					std::string chunk(available, '\0');
					DWORD read = 0;
					if(!WinHttpReadData(request, chunk.data(), available, &read) || text.size() + read > (1u << 20))
					{
						complete = false;
						break;
					}
					text.append(chunk.data(), read);
				}
				if(complete)
					body = std::move(text);
			}
		}
		if(request)
			WinHttpCloseHandle(request);
		if(connection)
			WinHttpCloseHandle(connection);
		WinHttpCloseHandle(session);
		return body;
#else
		return std::nullopt;
#endif
	}
}
