#pragma once

// What the external needs of the world to tell of a newer release (mdseqUpdates.h holds the parsing): the settings
// file, %LOCALAPPDATA%\md-drums-seq\settings.txt, shared by every set and device, and GitHub's API over HTTPS. Both
// are called off the main thread and Max's; nothing here touches Max.

#include <optional>
#include <string>

namespace mdseqUpdates
{
	struct Settings
	{
		bool enabled = true;	// "updates on|off": whether the device looks at all
		std::string lastCheck;	// "checked 2026-10-17": the day of the last answer
		std::string latest;		// "latest 0.2.0": the newest release that answer named, empty for none
	};

	Settings readSettings();
	void writeSettings(const Settings& _settings);

	// Today, as "2026-10-17", in local time.
	std::string today();

	// GitHub's answer to the releases' path (mdseq::ReleasesApiPath), none on any failure: no network, a status other
	// than 200, more than 1 MB. Waits 5 s at most for each step.
	std::optional<std::string> fetchReleases();
}
