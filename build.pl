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

my $buildCommandPrefix = '';
sub CheckInstallSDK
{
    print 'Setting up the Linux SDK';
    SDKDownloader::PrepareSDK('linux-sdk', '20180928', "artifacts");
    $buildCommandPrefix = "schroot -c $ENV{LINUX_BUILD_ENVIRONMENT} --";
}

my $err; # used by CheckFileError

my $mac = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF";
my $linuxD = "cmake -DCMAKE_BUILD_TYPE=Debug -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF -DRR_ENABLE_STATIC=ON -DUSE_VULKAN=OFF";
my $linuxR = "cmake -DCMAKE_BUILD_TYPE=Release -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=OFF -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=OFF -DRR_ENABLE_STATIC=ON -DUSE_VULKAN=OFF";
my $windows = "cmake -G \"Visual Studio 14 2015 Win64\" -DRR_USE_EMBREE=OFF -DRR_USE_OPENCL=ON -DRR_EMBED_KERNELS=ON -DRR_SAFE_MATH=ON -DRR_SHARED_CALC=ON -DCMAKE_PREFIX_PATH=3rdparty/opencl";

sub BuildRadeonRays
{
	my $cmakeString = shift;
    system("$buildCommandPrefix $cmakeString") && die("cmake failed");
	if ($Config{osname} eq "MSWin32")
	{
		system("\"C:/Program Files (x86)/Microsoft Visual Studio 14.0/Common7/IDE/devenv.exe\" RadeonRaysSDK.sln /Build Debug");
		system("\"C:/Program Files (x86)/Microsoft Visual Studio 14.0/Common7/IDE/devenv.exe\" RadeonRaysSDK.sln /Build RelWithDebInfo");
	}
	else
	{
		system("$buildCommandPrefix make") && die("Failed make");
	}
}

sub CopyHeaders
{
	mkpath('artifacts/include', {error => \ $err} );
	CheckFileError();
	dircopy("RadeonRays/include", "artifacts/include") or die("Failed to copy RadeonRays headers.");
	dircopy("Calc/inc", "artifacts/include") or die("Failed to copy Calc headers.");
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
mkpath('artifacts/lib', {error => \ $err} );
CheckFileError();
mkpath('builds', {error => \ $err} );
CheckFileError();

if ($Config{osname} eq "darwin")
{
	BuildRadeonRays($mac);
	mkpath('artifacts/lib/macOS', {error => \ $err} );
	CheckFileError();
	fcopy("bin/libRadeonRays.dylib", "artifacts/lib/macOS/libRadeonRays.dylib") or die "Copy of libRadeonRays.dylib failed: $!";
}

if ($Config{osname} eq "linux")
{
	CheckInstallSDK();
	BuildRadeonRays($linuxR);
	mkpath('artifacts/lib/linux', {error => \ $err} );
	CheckFileError();
	fcopy("RadeonRays/libRadeonRays.a", "artifacts/lib/linux/libRadeonRays.a") or die "Copy of libRadeonRays.a failed: $!";
	BuildRadeonRays($linuxD);
	fcopy("RadeonRays/libRadeonRaysD.a", "artifacts/lib/linux/libRadeonRaysD.a") or die "Copy of libRadeonRaysD.a failed: $!";
	system("rm -r artifacts/SDKDownloader") && die("Unable to clean up SDKDownloader directory.");
}

if ($Config{osname} eq "MSWin32")
{
	BuildRadeonRays($windows);
	
	# copy libs
	mkpath('artifacts/lib/Windows', {error => \ $err} );
	CheckFileError();
	fcopy("RadeonRays/RelWithDebInfo/RadeonRays.lib", "artifacts/lib/Windows/RadeonRays.lib") or die "Copy of RadeonRays.lib failed: $!";
	
	# copy dll files
	mkpath('artifacts/bin', {error => \ $err} );
	CheckFileError();
	mkpath('artifacts/bin/Windows', {error => \ $err} );
	CheckFileError();
	
	# Release
	fcopy("bin/RelWithDebInfo/Calc.dll", "artifacts/bin/Windows/Calc.dll") or die "Copy of Calc.dll failed: $!";
	fcopy("bin/RelWithDebInfo/Calc.pdb", "artifacts/bin/Windows/Calc.pdb") or die "Copy of Calc.pdb failed: $!";
	fcopy("bin/RelWithDebInfo/RadeonRays.dll", "artifacts/bin/Windows/RadeonRays.dll") or die "Copy of RadeonRays.dll failed: $!";
	fcopy("bin/RelWithDebInfo/RadeonRays.pdb", "artifacts/bin/Windows/RadeonRays.pdb") or die "Copy of RadeonRays.pdb failed: $!";
	
	# Debug
	fcopy("bin/Debug/CalcD.dll", "artifacts/bin/Windows/CalcD.dll") or die "Copy of CalcD.dll failed: $!";
	fcopy("bin/Debug/CalcD.pdb", "artifacts/bin/Windows/CalcD.pdb") or die "Copy of CalcD.pdb failed: $!";
	fcopy("bin/Debug/RadeonRaysD.dll", "artifacts/bin/Windows/RadeonRaysD.dll") or die "Copy of RadeonRaysD.dll failed: $!";
	fcopy("bin/Debug/RadeonRaysD.pdb", "artifacts/bin/Windows/RadeonRaysD.pdb") or die "Copy of RadeonRaysD.pdb failed: $!";
	
	# write build version.txt, only needed once as ACompleteBuild will combine all artifacts.
	my $branch = qx("git symbolic-ref -q HEAD");
	my $revision = qx("git rev-parse HEAD");
	open(BUILD_INFO_FILE, '>', "artifacts/version.txt") or die("Unable to write build information to version.txt");
	print BUILD_INFO_FILE "$branch";
	print BUILD_INFO_FILE "$revision";
	close(BUILD_INFO_FILE);
}

CopyHeaders();