#!/usr/bin/env perl -w
use File::Path;
use File::Copy::Recursive qw(fcopy dircopy);
use Config;
use Archive::Zip;

my $err;

my $mac = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF";
my $linux = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF";
my $windows = "cmake -G \"Visual Studio 14 2015 Win64\" -DRR_USE_EMBREE=ON -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=ON -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=ON";

sub BuildRadeonRays
{
	my $makeString = shift;
    #system("make clean") && die("Clean failed");
    system($makeString) && die("cmake failed");
    system("make") && die("Failed make");
}

sub ZipIt
{
	# copy headers
	mkpath('builds/include', {error => \ $err} );
	CheckFileError();
	dircopy("RadeonRays/include", "builds/include") or die("Failed to copy RadeonRays headers.");
	dircopy("Calc/inc", "builds/include") or die("Failed to copy Calc headers.");

	# write build version.txt
	my $branch = qx("git symbolic-ref -q HEAD");
	my $revision = qx("git rev-parse HEAD");
	open(BUILD_INFO_FILE, '>', "builds/version.txt") or die("Unable to write build information to version.txt");
	print BUILD_INFO_FILE "$branch";
	print BUILD_INFO_FILE "$revision";
	close(BUILD_INFO_FILE);

	# create builds.zip
	my $zip = Archive::Zip->new();
	$zip->addTree( './builds', '' );
	$zip->writeToFileNamed('builds.zip');
	
	#system("rm -r builds") && die("Unable to clean up builds directory.");
}

sub CheckFileError
{
	if (@$err) 
	{
		for my $diag (@$err) 
		{
			my ($file, $message) = %$diag;
			if ($file eq '') 
			{
			  die("general error: $message\n");
			}
			else 
			{
			  die("problem unlinking $file: $message\n");
			}
		}
	}
}

mkpath('builds', {error => \ $err} );
CheckFileError();
mkpath('builds/lib', {error => \ $err} );
CheckFileError();

if ($Config{osname} eq "darwin")
{
	BuildRadeonRays($mac);
	fcopy("build/libRadeonRays.dylib", "lib/macOS/libRadeonRays.dylib") or die "Copy of libRadeonRays.dylib failed: $!";
}

if ($Config{osname} eq "linux")
{
	BuildRadeonRays($linux);
	fcopy("build/libRadeonRays.a", "lib/linux/libRadeonRays.a") or die "Copy of libRadeonRays.a failed: $!";
}

if ($Config{osname} eq "MSWin32")
{
	BuildRadeonRays($windows);
	
	# copy libs
	fcopy("RadeonRays/RelWithDebInfo/RadeonRays.lib", "lib/Windows/RadeonRays.lib") or die "Copy of RadeonRays.lib failed: $!";
	
	# copy dll files
	mkpath('builds/bin', {error => \ $err} );
	CheckFileError();
	fcopy("/Bin/RelWithDebInfo/x64/Calc.dll", "bin/Windows/Calc.dll") or die "Copy of Calc.dll failed: $!";
	fcopy("/Bin/RelWithDebInfo/x64/Calc.pdb", "bin/Windows/Calc.pdb") or die "Copy of Calc.pdb failed: $!";
	fcopy("/Bin/RelWithDebInfo/x64/RadeonRays.dll", "bin/Windows/RadeonRays.dll") or die "Copy of RadeonRays.dll failed: $!";
	fcopy("/Bin/RelWithDebInfo/x64/RadeonRays.pdb", "bin/Windows/RadeonRays.pdb") or die "Copy of RadeonRays.pdb failed: $!";	
	fcopy("3rdparty/embree/bin/x64/embree.dll", "bin/Windows/embree.dll") or die "Copy of embree.dll failed: $!";
	fcopy("3rdparty/embree/bin/x64/tbb.dll", "bin/Windows/tbb.dll") or die "Copy of tbb.dll failed: $!";
}

ZipIt();