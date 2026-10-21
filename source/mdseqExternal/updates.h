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

	// The file is read and written under one lock, and written whole to a file renamed into place: the devices'
	// checks, on threads of their own, and the window's switch, on the main thread, never read half a file nor lose
	// each other's writes.
	Settings readSettings();
	// The switch, kept for every set.
	void setEnabled(bool _on);

	// The settings once GitHub is asked for the newest release, at most once a day (by local time) for every device
	// and set: the day's answer is kept with the day. One check runs at a time in the process, so that the devices of
	// a set that opens ask once. GitHub's answer to the releases' path (mdseq::ReleasesApiPath) counts only whole: no
	// network, a status other than 200 or more than 1 MB leave the settings as they were, to ask again next time.
	// Each step waits 5 s at most. Nothing is asked while the switch is off.
	Settings checkOncePerDay();
}
