#!/usr/bin/env perl -w
use Cwd qw(getcwd);
use File::Path;

my $pathToLib;
BEGIN { $pathToLib = getcwd . '/3rdparty/Perl/lib' }
use lib $pathToLib;
use File::Copy::Recursive qw(fcopy dircopy);
use Config;
use Archive::Zip;
use SDKDownloader;

sub CheckInstallSDK
{
    print 'Setting up the Linux SDK';
    SDKDownloader::PrepareSDK('linux-sdk', '20180907', "artifacts");
}

my $err; # used by CheckFileError

my $mac = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF";
my $linux = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=ON -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=ON -DRR_ENABLE_STATIC=ON";
my $windows = "cmake -G \"Visual Studio 14 2015 Win64\" -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=ON -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=ON -DCMAKE_PREFIX_PATH=3rdparty/opencl";

sub BuildRadeonRays
{
	my $cmakeString = shift;
    system($cmakeString) && die("cmake failed");
	if ($Config{osname} eq "MSWin32")
	{
		system("\"C:/Program Files (x86)/Microsoft Visual Studio 14.0/Common7/IDE/devenv.exe\" RadeonRaysSDK.sln /Build RelWithDebInfo");
	}
	else
	{
		system("make") && die("Failed make");
	}
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
	$zip->writeToFileNamed('artifacts/builds.zip');
	
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


mkpath('artifacts', {error => \ $err} );
CheckFileError();
mkpath('builds', {error => \ $err} );
CheckFileError();
mkpath('builds/lib', {error => \ $err} );
CheckFileError();

if ($Config{osname} eq "darwin")
{
	BuildRadeonRays($mac);
	mkpath('builds/lib/macOS', {error => \ $err} );
	CheckFileError();
	fcopy("bin/libRadeonRays.dylib", "builds/lib/macOS/libRadeonRays.dylib") or die "Copy of libRadeonRays.dylib failed: $!";
}

if ($Config{osname} eq "linux")
{
	CheckInstallSDK();
	BuildRadeonRays($linux);
	mkpath('builds/lib/linux', {error => \ $err} );
	CheckFileError();
	fcopy("RadeonRays/libRadeonRays.a", "builds/lib/linux/libRadeonRays.a") or die "Copy of libRadeonRays.a failed: $!";
}

if ($Config{osname} eq "MSWin32")
{
	BuildRadeonRays($windows);
	
	# copy libs
	mkpath('builds/lib/Windows', {error => \ $err} );
	CheckFileError();
	fcopy("RadeonRays/RelWithDebInfo/RadeonRays.lib", "builds/lib/Windows/RadeonRays.lib") or die "Copy of RadeonRays.lib failed: $!";
	
	# copy dll files
	mkpath('builds/bin', {error => \ $err} );
	CheckFileError();
	mkpath('builds/bin/Windows', {error => \ $err} );
	CheckFileError();
	fcopy("bin/RelWithDebInfo/Calc.dll", "builds/bin/Windows/Calc.dll") or die "Copy of Calc.dll failed: $!";
	fcopy("bin/RelWithDebInfo/Calc.pdb", "builds/bin/Windows/Calc.pdb") or die "Copy of Calc.pdb failed: $!";
	fcopy("bin/RelWithDebInfo/RadeonRays.dll", "builds/bin/Windows/RadeonRays.dll") or die "Copy of RadeonRays.dll failed: $!";
	fcopy("bin/RelWithDebInfo/RadeonRays.pdb", "builds/bin/Windows/RadeonRays.pdb") or die "Copy of RadeonRays.pdb failed: $!";	
	#fcopy("3rdparty/embree/bin/x64/embree.dll", "builds/bin/Windows/embree.dll") or die "Copy of embree.dll failed: $!";
	#fcopy("3rdparty/embree/bin/x64/tbb.dll", "builds/bin/Windows/tbb.dll") or die "Copy of tbb.dll failed: $!";
}

ZipIt();