// A newer release: versions read from tags, the first release GitHub's API lists, the strip's line when it is newer
// than this build, and whether a check is due today.

#include "mdseqUpdates.h"
#include "mdseqTestSupport.h"

#include <cstdio>

namespace
{
	using namespace mdseq;

	void versions()
	{
		CHECK(parseVersion("v0.2.0") && parseVersion("v0.2.0")->minor == 2);
		CHECK(parseVersion("1.10.3") && parseVersion("1.10.3")->minor == 10 && parseVersion("1.10.3")->patch == 3);
		CHECK(!parseVersion("") && !parseVersion("v1.2") && !parseVersion("1.2.x") && !parseVersion("md-drums-v0.1.1"));
		CHECK(*parseVersion("0.1.0") < *parseVersion("0.1.1"));
		CHECK(*parseVersion("0.9.9") < *parseVersion("0.10.0"));
		CHECK(!(*parseVersion("1.0.0") < *parseVersion("0.9.9")));
		CHECK(versionText(*parseVersion("v2.3.4")) == "2.3.4");
	}

	// GitHub's /releases answer: an array, newest first; the first tag_name names it.
	void releases()
	{
		const char* answer = "[{\"url\":\"x\",\"tag_name\": \"v0.2.0\",\"prerelease\":true},{\"tag_name\":\"v0.1.0\"}]";
		CHECK(firstReleaseVersion(answer) && versionText(*firstReleaseVersion(answer)) == "0.2.0");
		CHECK(!firstReleaseVersion("[]") && !firstReleaseVersion("{\"message\":\"Not Found\"}"));
		CHECK(!firstReleaseVersion("[{\"tag_name\":\"latest\"}]"));
	}

	void notice()
	{
		const auto newer = parseVersion("0.2.0");
		CHECK(updateNotice(newer, "0.1.0") ==
			"md-drums-seq 0.2.0 is available: github.com/c0remusic/md-drums-seq/releases");
		CHECK(updateNotice(newer, "0.2.0").empty() && updateNotice(newer, "0.3.0").empty());
		CHECK(updateNotice(std::nullopt, "0.1.0").empty());
	}

	// At most once a day: a check is due unless the last one was today.
	void dueOncePerDay()
	{
		CHECK(checkDue("", "2026-10-17") && checkDue("2026-10-16", "2026-10-17"));
		CHECK(!checkDue("2026-10-17", "2026-10-17"));
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	versions();
	releases();
	notice();
	dueOncePerDay();
	return mdseqTest::finish("mdseqUpdatesTest");
}
