#pragma once

// A newer release of md-drums-seq: what the adapter reads from GitHub's API, at most once a day, worded as the line
// the strip shows. Reading the network and the settings file is the adapter's; this part only parses and compares.

#include <optional>
#include <string>
#include <string_view>

namespace mdseq
{
	// The public repository whose releases the device looks at, as GitHub's API path and as the address shown.
	constexpr const char* ReleasesApiPath = "/repos/c0remusic/md-drums-seq/releases?per_page=1";
	constexpr const char* ReleasesPage = "github.com/c0remusic/md-drums-seq/releases";

	struct Version
	{
		int major = 0;
		int minor = 0;
		int patch = 0;

		bool operator<(const Version& _other) const;
	};

	// "1.2.3" or "v1.2.3", none for anything else.
	std::optional<Version> parseVersion(std::string_view _text);
	std::string versionText(const Version& _version);

	// The version of the first release a /releases answer lists (the newest), none in an answer without one.
	std::optional<Version> firstReleaseVersion(std::string_view _json);

	// "md-drums-seq 0.2.0 is available: github.com/..." when the release is newer than the build, empty otherwise.
	std::string updateNotice(const std::optional<Version>& _release, std::string_view _current);

	// Whether a check is due: not when the last one was today (dates as "2026-10-17").
	bool checkDue(std::string_view _lastDay, std::string_view _today);
}
