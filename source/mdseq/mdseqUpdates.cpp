#include "mdseqUpdates.h"

#include <tuple>

namespace mdseq
{
	bool Version::operator<(const Version& _other) const
	{
		return std::tie(major, minor, patch) < std::tie(_other.major, _other.minor, _other.patch);
	}

	std::optional<Version> parseVersion(std::string_view _text)
	{
		if(!_text.empty() && _text.front() == 'v')
			_text.remove_prefix(1);
		int parts[3] = {};
		for(int i = 0; i < 3; ++i)
		{
			if(_text.empty() || _text.front() < '0' || _text.front() > '9')
				return std::nullopt;
			int n = 0;
			while(!_text.empty() && _text.front() >= '0' && _text.front() <= '9')
			{
				n = n * 10 + (_text.front() - '0');
				if(n > 9999)
					return std::nullopt;
				_text.remove_prefix(1);
			}
			parts[i] = n;
			if(i < 2)
			{
				if(_text.empty() || _text.front() != '.')
					return std::nullopt;
				_text.remove_prefix(1);
			}
		}
		if(!_text.empty())
			return std::nullopt;
		return Version{parts[0], parts[1], parts[2]};
	}

	std::string versionText(const Version& _version)
	{
		return std::to_string(_version.major) + "." + std::to_string(_version.minor) + "." +
			std::to_string(_version.patch);
	}

	std::optional<Version> firstReleaseVersion(const std::string_view _json)
	{
		// "tag_name": "v0.2.0", spaces or none around the colon.
		const size_t key = _json.find("\"tag_name\"");
		if(key == std::string_view::npos)
			return std::nullopt;
		const size_t open = _json.find('"', _json.find(':', key));
		if(open == std::string_view::npos)
			return std::nullopt;
		const size_t close = _json.find('"', open + 1);
		if(close == std::string_view::npos)
			return std::nullopt;
		return parseVersion(_json.substr(open + 1, close - open - 1));
	}

	std::string updateNotice(const std::optional<Version>& _release, const std::string_view _current)
	{
		const std::optional<Version> current = parseVersion(_current);
		if(!_release || !current || !(*current < *_release))
			return {};
		return "md-drums-seq " + versionText(*_release) + " is available: " + ReleasesPage;
	}

	bool checkDue(const std::string_view _lastDay, const std::string_view _today)
	{
		return _lastDay != _today;
	}
}
