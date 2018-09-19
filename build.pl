#!/usr/bin/env perl -w
use File::Path;
use Config;

print "$Config{osname}\n";

my $mac = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF";
my $linux = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF";
my $windows = "cmake -G \"Visual Studio 14 2015 Win64\" -DRR_USE_EMBREE=ON -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=ON -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=ON";

sub BuildRadeonRays
{
	my $makeString = shift;
	#system("mkdir build");
    system("make clean") && die("Clean failed");
    system($makeString) && die("cmake failed");
    system("make") && die("Failed make");
}

sub ZipIt
{
	system("mkdir -p build/include") && die("Failed to create directory.");

	# write build version.txt
	my $git_info = qx(git symbolic-ref -q HEAD && git rev-parse HEAD);
	open(BUILD_INFO_FILE, '>', "build/version.txt") or die("Unable to write build information to version.txt");
	print BUILD_INFO_FILE "$git_info";
	close(BUILD_INFO_FILE);

	# create zip
	system("cp build/$api/source/*.h build/include") && die("Failed to copy headers.");
	system("zip ../builds.zip -r build.txt lib include") && die("Failed to package zip file.");
	system("rm -r build") && die("Unable to clean up directory.");
}

if ($Config{osname} eq "macOS")
{
	BuildRadeonRays($mac);
}

if ($Config{osname} eq "linux")
{
	BuildRadeonRays($linux);
}

if ($Config{osname} eq "MSWin32")
{
	BuildRadeonRays($windows);
}

ZipIt();