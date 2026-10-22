#include "updates.h"

#include "mdseqUpdates.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#elif defined(__APPLE__)
#include <curl/curl.h>
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

		std::mutex& fileLock()
		{
			static std::mutex lock;
			return lock;
		}

		Settings read()
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

		void write(const Settings& _settings)
		{
			const std::filesystem::path path = settingsPath();
			std::filesystem::path written = path;
			written += ".new";
			std::error_code error;
			std::filesystem::create_directories(path.parent_path(), error);
			{
				std::ofstream file(written, std::ios::trunc);
				file << "updates " << (_settings.enabled ? "on" : "off") << "\n";
				if(!_settings.lastCheck.empty())
					file << "checked " << _settings.lastCheck << "\n";
				if(!_settings.latest.empty())
					file << "latest " << _settings.latest << "\n";
				if(!file.flush())
					return;
			}
			std::filesystem::rename(written, path, error);
		}

		// Today, as "2026-10-17", in local time.
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
			HINTERNET session = WinHttpOpen(L"MModuleSeq", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS, 0);
			if(!session)
				return body;
			WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
			HINTERNET connection = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
			HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET",
				wide(mdseq::ReleasesApiPath).c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
				WINHTTP_FLAG_SECURE) : nullptr;
			if(request && WinHttpSendRequest(request, L"Accept: application/vnd.github+json\r\n",
				static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr))
			{
				DWORD status = 0;
				DWORD size = sizeof(status);
				WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
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
#elif defined(__APPLE__)
			// macOS ships libcurl: the same request, the same limits.
			CURL* curl = curl_easy_init();
			if(!curl)
				return std::nullopt;
			std::string text;
			const auto append = +[](char* _data, size_t _size, size_t _count, void* _text) -> size_t
			{
				auto& t = *static_cast<std::string*>(_text);
				if(t.size() + _size * _count > (1u << 20))
					return 0;	// stops the transfer: more than 1 MB
				t.append(_data, _size * _count);
				return _size * _count;
			};
			const std::string url = std::string("https://api.github.com") + mdseq::ReleasesApiPath;
			curl_slist* headers = curl_slist_append(nullptr, "Accept: application/vnd.github+json");
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "MModuleSeq");
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &text);
			long status = 0;
			const CURLcode result = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			if(result != CURLE_OK || status != 200)
				return std::nullopt;
			return text;
#else
			return std::nullopt;
#endif
		}
	}

	Settings readSettings()
	{
		std::lock_guard<std::mutex> guard(fileLock());
		return read();
	}

	void setEnabled(const bool _on)
	{
		std::lock_guard<std::mutex> guard(fileLock());
		Settings s = read();
		s.enabled = _on;
		write(s);
	}

	Settings checkOncePerDay()
	{
		static std::mutex checking;
		std::lock_guard<std::mutex> one(checking);
		Settings s = readSettings();
		const std::string day = today();
		if(!s.enabled || !mdseq::checkDue(s.lastCheck, day))
			return s;
		const std::optional<std::string> answer = fetchReleases();
		if(!answer)
			return s;
		const std::optional<mdseq::Version> release = mdseq::firstReleaseVersion(*answer);
		// Read again: the switch may have moved while GitHub answered.
		std::lock_guard<std::mutex> guard(fileLock());
		s = read();
		s.lastCheck = day;
		s.latest = release ? mdseq::versionText(*release) : std::string();
		write(s);
		return s;
	}
}
