#!/usr/bin/env python3
import argparse, subprocess, sys

#this builds successfully if using NDK 23c

# Parse our command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument("--upload", default=None, help="Upload built package to the specified remote")
parser.add_argument("--android", action="store_true", help="Use to do an Android build")
parser.add_argument("--android_conan_profile_host", default="ue4.27-Android-armv8-unknown-linux-gnu_3",
                    help="Name of host profile to use for Android (only used if --android argument is given). This profile refers to the build target (so Android). Corresponds to the --profile:host conan argument.")
parser.add_argument("--android_conan_profile_build", default="ue4.27-Linux-x86_64-unknown-linux-gnu",
                    help="Name of build profile to use for Android (only used if --android argument is given). This profile refers to the OS/machine you are building on (Linux/Windows/MacOS). Corresponds to the --profile:build conan argument.")
args = parser.parse_args()

# Query ue4cli for the UE4 version string
proc = subprocess.Popen(["ue4", "version", "short"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
(stdout, stderr) = proc.communicate(input)
if proc.returncode != 0:
    raise Exception("failed to retrieve UE4 version string")

# Build the Conan package, using the Engine version as the channel name
channel = stdout.strip()
if args.android:
    if subprocess.call(["conan", "create", ".", "adamrehn/{}".format(channel), "--profile:build", "ue4", "--profile:host", args.android_conan_profile_host#,]) != 0:
        #"--build", "UElibPNG", "--build", "libjpeg",
                        #"--build", "zlib", "--build", "opencv-ue4"
                           #, "--build", "ue4lib"
                        ]) != 0:
                        #"--build", "UElibPNG", "--build", "libjpeg", "--build", "zlib"]) != 0:
        sys.exit(1)
else:
    if subprocess.call(["conan", "create", ".", "adamrehn/{}".format(channel), "--profile", "ue4"]) != 0:
        sys.exit(1)

# Upload the package to the specified remote if the user provided one
if args.upload != None:
    if subprocess.call(["conan", "upload", "opencv-ue4/*@adamrehn/*", "--all", "--confirm", "-r", args.upload]) != 0:
        sys.exit(1)
