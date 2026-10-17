// The Kit as MD Drums tells it (ADR 0003): the file read, its version and the lines skipped; the hello's bytes and its
// pace; when the Kit read changes.

#include "mdseqKit.h"
#include "mdseqTestSupport.h"

#include <cstdio>
#include <string>
#include <vector>

namespace
{
	using namespace mdseq;

	const char* const File =
		"md-drums-kit 1\r\n"
		"kit TRX UW\r\n"
		"track 1 16 TRX-BD PTCH DEC RAMP RDEC STRT NOIS HARM CLIP\r\n"
		"track 4 19 TRX-CP CLPY TONE HARD RICH RATE ROOM RSIZ RTUN\r\n"
		"values 1 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 127\r\n"
		"lfo 1 2 3\r\n";

	void readsAFile()
	{
		const std::optional<Kit> kit = parseKit(File);
		CHECK(kit.has_value());
		if(!kit)
			return;
		CHECK(kit->name == "TRX UW");
		const KitTrack& bd = kit->tracks[0];
		CHECK(bd.machine == 16 && bd.machineName == "TRX-BD");
		CHECK(bd.synNames[0] == "PTCH" && bd.synNames[7] == "CLIP");
		CHECK(bd.values[0] == 10 && bd.values[22] == 32 && bd.values[23] == 127);
		const KitTrack& cp = kit->tracks[3];
		CHECK(cp.machine == 19 && cp.machineName == "TRX-CP" && cp.synNames[2] == "HARD");
		CHECK(cp.values[0] == -1);	// no values line
		// A Track MD Drums says nothing of
		CHECK(kit->tracks[1].machine == -1 && kit->tracks[1].machineName.empty() && kit->tracks[1].values[5] == -1);
	}

	void skipsWhatItDoesNotUnderstand()
	{
		// Another version, no header, nothing: no Kit.
		CHECK(!parseKit("md-drums-kit 2\nkit X\n"));
		CHECK(!parseKit("kit X\ntrack 1 16 TRX-BD A B C D E F G H\n"));
		CHECK(!parseKit(""));

		// Lines it cannot read leave the rest: a Track out of range, a Machine out of range, a line too short, values
		// out of range or missing, a SYN name missing.
		const std::optional<Kit> kit = parseKit(
			"md-drums-kit 1\n"
			"track 17 16 TRX-BD A B C D E F G H\n"
			"track 2 192 TRX-BD A B C D E F G H\n"
			"track 3 16 TRX-BD A B C\n"
			"values 2 1 2 3\n"
			"values 3 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 128\n"
			"track 5 17 TRX-SD - TUNE DEC - SNAP NOIS HARM CLIP\n"
			"kit\n");
		CHECK(kit.has_value());
		if(!kit)
			return;
		for(int t = 0; t < 4; ++t)
			CHECK(kit->tracks[static_cast<size_t>(t)] == KitTrack{});
		CHECK(kit->tracks[4].machineName == "TRX-SD" && kit->tracks[4].synNames[0] == "-" &&
			kit->tracks[4].synNames[1] == "TUNE");
		CHECK(kit->name.empty());
	}

	void helloBytes()
	{
		// 0xABCDEF: 101010 111100 110111 101111, each six bits plus one as a velocity.
		const auto bytes = hello(0xABCDEF);
		const std::vector<MidiBytes> expected = {{0x90, 124, 43}, {0x80, 124, 0}, {0x90, 125, 61}, {0x80, 125, 0},
			{0x90, 126, 56}, {0x80, 126, 0}, {0x90, 127, 48}, {0x80, 127, 0}};
		CHECK(std::vector<MidiBytes>(bytes.begin(), bytes.end()) == expected);
	}

	void ids()
	{
		CHECK(KitLink(0xABCDEF).id() == 0xABCDEF);
		CHECK(KitLink(0x12ABCDEF).id() == 0xABCDEF);	// 24 bits
		CHECK(KitLink(0xABCDEF).fileName() == "abcdef.kit");
		CHECK(KitLink(0x00000F).fileName() == "00000f.kit");
		// Never 0: MD Drums would hold no id.
		CHECK(KitLink(0).id() != 0 && KitLink(0x5000000).id() != 0);
		CHECK(KitLink(0).id() <= 0xFFFFFF && KitLink(0x5000000).id() <= 0xFFFFFF);
	}

	// A hello at the first tick, then every second (two ticks) while there is no file, every five seconds (ten ticks)
	// while there is one, busy or read, and every second again once it has gone.
	void helloCadence()
	{
		HelloPace pace;
		std::vector<int> hellos;
		for(int tick = 1; tick <= 30; ++tick)
		{
			KitFile::State file = KitFile::State::Absent;
			if(tick >= 4 && tick <= 23)
				file = tick % 3 == 0 ? KitFile::State::Busy : KitFile::State::Read;
			if(pace.tick(file))
				hellos.push_back(tick);
		}
		CHECK((hellos == std::vector<int>{1, 3, 13, 23, 25, 27, 29}));
	}

	void kitChanges()
	{
		KitLink link(1);
		CHECK(!link.poll(KitFile::none()));
		CHECK(!link.kit());
		CHECK(link.poll(KitFile::read(File)));
		CHECK(link.kit() && link.kit()->name == "TRX UW");
		CHECK(!link.poll(KitFile::read(File)));
		// Another text that says the same: an unknown line more.
		CHECK(!link.poll(KitFile::read(std::string(File) + "links 1 2\n")));
		std::string other = File;
		other.replace(other.find("TRX UW"), 6, "E12 UW");
		CHECK(link.poll(KitFile::read(other)));
		CHECK(link.kit()->name == "E12 UW");
		// A file this build cannot read is no Kit.
		CHECK(link.poll(KitFile::read("md-drums-kit 2\n")));
		CHECK(!link.kit());
		CHECK(link.poll(KitFile::read(File)));
		CHECK(link.poll(KitFile::none()));
		CHECK(!link.kit());
	}

	// A file that MD Drums is replacing does not open for a moment (seen in Live on 2026-10-17: the Track names went
	// back to numbers for half a second): the Kit stays until a poll reads the file again or finds it gone.
	void busyFileKeepsTheKit()
	{
		KitLink link(1);
		link.poll(KitFile::read(File));
		for(int poll = 1; poll <= 12; ++poll)
		{
			CHECK(!link.poll(KitFile::busy()));
			CHECK(link.kit() && link.kit()->name == "TRX UW");
		}
		CHECK(!link.poll(KitFile::read(File)));
		CHECK(!link.poll(KitFile::busy()));
		CHECK(link.poll(KitFile::none()));
		CHECK(!link.kit());
		// Busy before any file: still no Kit.
		KitLink fresh(2);
		CHECK(!fresh.poll(KitFile::busy()));
		CHECK(!fresh.kit());
	}
}

int main()
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	readsAFile();
	skipsWhatItDoesNotUnderstand();
	helloBytes();
	ids();
	helloCadence();
	kitChanges();
	busyFileKeepsTheKit();
	return mdseqTest::finish("mdseqKitTest");
}
