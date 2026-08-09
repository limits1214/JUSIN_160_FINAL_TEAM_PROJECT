#include "pch.h"
#include "Importer.h"


using namespace std;

int main(int argc, char* argv[])
{
	shared_ptr<CImporter> import = make_shared<CImporter>();

	// Targeted static conversion used by the verified asset pipeline.
	// The input directory must be named Static so the existing output layout is
	// preserved: <root>/Models/Static/<model>.bin.
	if (argc > 1 && std::string(argv[1]) == "--static-folder")
	{
		if (argc != 3)
		{
			cerr << "Usage: Model_Binarization.exe --static-folder <OriginData/Static>\n";
			return 1;
		}
		if (FAILED(import->ImportFBXFolder("", argv[2])))
		{
			cerr << "Static-folder conversion failed.\n";
			return 1;
		}
		return 0;
	}

	// Targeted skeletal conversion. This keeps the existing OriginData layout:
	// <root>/Models/OriginData/Skeleton -> <root>/Models/Skeleton/<model>/.
	if (argc > 1 && std::string(argv[1]) == "--skeletal-folder")
	{
		if (argc != 3)
		{
			cerr << "Usage: Model_Binarization.exe --skeletal-folder <OriginData/Skeleton>\n";
			return 1;
		}
		if (FAILED(import->ImportFBXFolder("", argv[2])))
		{
			cerr << "Skeletal-folder conversion failed.\n";
			return 1;
		}
		return 0;
	}

	// Dedicated whole-map conversion. The existing no-argument batch behavior is
	// intentionally kept unchanged.
	// Model_Binarization.exe --whole-map <input.fbx> <output-dir> <chunk-size>
	if (argc > 1 && std::string(argv[1]) == "--whole-map")
	{
		if (argc != 5)
		{
			cerr << "Usage: Model_Binarization.exe --whole-map <input.fbx> <output-dir> <chunk-size>\n";
			return 1;
		}

		float chunkSize = 0.f;
		try
		{
			chunkSize = std::stof(argv[4]);
		}
		catch (...)
		{
			cerr << "chunk-size must be a positive number.\n";
			return 1;
		}

		if (chunkSize <= 0.f)
		{
			cerr << "chunk-size must be greater than zero.\n";
			return 1;
		}

		if (FAILED(import->ImportWholeMapFBX(argv[2], argv[3], chunkSize)))
		{
			cerr << "Whole-map conversion failed.\n";
			return 1;
		}

		return 0;
	}

	import->ImportFBXFolder("","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/Skeleton/");
	//import->ImportFBXFolder_ForMapJson("","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/Skeleton","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/StaticModelJson/");
	//import->ImportFBXFolder_ForMapJson("","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/Static/ParticleMeshes","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/ParticleModelJson");
	return 0;
}
