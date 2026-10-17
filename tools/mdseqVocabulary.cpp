// Writes the vocabulary between the device's patch and the external (source/mdseq/mdseqProtocol.h) as JSON, for
// make_device.py, which builds the patch from it, and check_device.py, which checks the device against it.
//
// Run: mdseqVocabulary <output.json>

#include "mdseqProtocol.h"

#include <cstdio>
#include <fstream>

int main(const int _argc, char** _argv)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	if(_argc != 2)
	{
		std::fprintf(stderr, "usage: mdseqVocabulary <output.json>\n");
		return 2;
	}
	std::ofstream out(_argv[1], std::ios::binary);
	out << mdseq::vocabularyJson();
	out.close();
	if(!out)
	{
		std::fprintf(stderr, "mdseqVocabulary: cannot write %s\n", _argv[1]);
		return 1;
	}
	return 0;
}
