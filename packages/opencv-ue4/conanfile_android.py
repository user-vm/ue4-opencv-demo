#this builds successfully if using NDK 23c
#if using NDK 21e, fails because of zlib linking error (see https://github.com/android/ndk/issues/1179 )
from conans import ConanFile, CMake, tools

class OpenCVUE4Conan(ConanFile):
    name = "opencv-ue4"
    version = "3.3.0"
    url = "https://github.com/adamrehn/ue4-opencv-demo"
    description = "OpenCV custom build for UE4"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = (
        "libcxx/ue4@adamrehn/profile",
        "ue4util/ue4@adamrehn/profile"#,
        #"gstreamer/1.19.2"
    )
    
    def requirements(self):
        pass
        #needed if building for Linux, not Android
        #self.requires("zlib/ue4@adamrehn/{}".format(self.channel))
        #self.requires("UElibPNG/ue4@adamrehn/{}".format(self.channel))
        #self.requires("libjpeg/9d")
        #self.requires("UElibJPG/ue4@adamrehn/{}".format(self.channel))

        #not used because of weird license, using v4l2 to set webcam settings instead
        ##self.requires("gstreamer/1.19.2")
    
    def cmake_flags(self):
        flags = [
            #enable opencv-contrib modules
            "-DOPENCV_EXTRA_MODULES_PATH=opencv_contrib/modules", #opencv-contrib modules (ex. for Aruco marker detection)

            # Disable a whole bunch of external dependencies
            "-DENABLE_IMPL_COLLECTION=OFF", "-DENABLE_INSTRUMENTATION=OFF", "-DENABLE_NOISY_WARNINGS=OFF",
            "-DENABLE_POPCNT=OFF", "-DENABLE_SOLUTION_FOLDERS=ON", "-DINSTALL_CREATE_DISTRIB=OFF",
            "-DINSTALL_C_EXAMPLES=OFF", "-DINSTALL_PYTHON_EXAMPLES=OFF", "-DINSTALL_TESTS=OFF",
            "-DBUILD_SHARED_LIBS=OFF", "-DBUILD_TESTS=OFF","-DBUILD_PERF_TESTS=OFF", 
            "-DBUILD_opencv_python2=OFF", "-DBUILD_opencv_python3=OFF", "-DBUILD_CUDA_STUBS=OFF",
            "-DWITH_CLP=OFF", "-DWITH_CSTRIPES=OFF", "-DWITH_CUBLAS=OFF", "-DWITH_CUDA=OFF",
            "-DWITH_CUFFT=OFF", "-DWITH_DIRECTX=OFF", "-DWITH_DSHOW=OFF", "-DWITH_EIGEN=OFF",
            "-DWITH_FFMPEG=OFF", "-DWITH_GDAL=OFF", "-DWITH_GDCM=OFF", "-DWITH_GIGEAPI=OFF",
            "-DWITH_GSTREAMER=OFF", "-DWITH_GSTREAMER_0_10=OFF", "-DWITH_INTELPERC=OFF",
            "-DWITH_IPP=OFF", "-DWITH_IPP_A=OFF", "-DWITH_JASPER=OFF", "-DWITH_JPEG=ON",
            "-DWITH_LAPACK=OFF", "-DWITH_MATLAB=OFF", "-DWITH_MSMF=OFF", "-DWITH_OPENCL=ON",
            "-DWITH_OPENCLAMDBLAS=OFF", "-DWITH_OPENCLAMDFFT=OFF", "-DWITH_OPENCL_SVM=OFF",
            "-DWITH_OPENEXR=OFF", "-DWITH_OPENGL=OFF", "-DWITH_OPENMP=OFF", "-DWITH_OPENNI=OFF",
            "-DWITH_OPENNI2=OFF", "-DWITH_OPENVX=OFF", "-DWITH_PVAPI=OFF", "-DWITH_QT=OFF",
            "-DWITH_TBB=OFF", "-DWITH_TIFF=OFF", "-DWITH_VFW=OFF", "-DWITH_VTK=OFF",
            "-DWITH_WEBP=OFF", "-DWITH_WIN32UI=OFF", "-DWITH_XIMEA=OFF", "-DWITH_ITT=OFF",
            "-DBUILD_WITH_STATIC_CRT=OFF", "-DWITH_CAROTENE=OFF", #carotene fails on Android builds. On other platforms, OpenCV turns it off regardless.
            
            # Just build a few core modules
            "-DBUILD_opencv_apps=OFF", "-DBUILD_opencv_calib3d=ON", "-DBUILD_opencv_core=ON",
            "-DBUILD_opencv_cudaarithm=OFF", "-DBUILD_opencv_cudabgsegm=OFF", "-DBUILD_opencv_cudacodec=OFF",
            "-DBUILD_opencv_cudafeatures2d=OFF", "-DBUILD_opencv_cudafilters=OFF", "-DBUILD_opencv_cudaimgproc=OFF",
            "-DBUILD_opencv_cudalegacy=OFF", "-DBUILD_opencv_cudaobjdetect=OFF", "-DBUILD_opencv_cudaoptflow=OFF",
            "-DBUILD_opencv_cudastereo=OFF", "-DBUILD_opencv_cudawarping=OFF", "-DBUILD_opencv_cudev=OFF",
            "-DBUILD_opencv_dnn=OFF", "-DBUILD_opencv_features2d=ON", "-DBUILD_opencv_flann=ON",
            "-DBUILD_opencv_highgui=ON", "-DBUILD_opencv_imgcodecs=ON", "-DBUILD_opencv_imgproc=ON",
            "-DBUILD_opencv_ml=OFF", "-DBUILD_opencv_objdetect=OFF", "-DBUILD_opencv_photo=OFF",
            "-DBUILD_opencv_shape=ON", "-DBUILD_opencv_stitching=OFF", "-DBUILD_opencv_superres=OFF",
            "-DBUILD_opencv_ts=OFF", "-DBUILD_opencv_video=OFF", "-DBUILD_opencv_videoio=ON",
            "-DBUILD_opencv_videostab=OFF", "-DBUILD_opencv_world=OFF",

            #needed for Linux build, trying without for Android
            #"-DWITH_PNG=ON",
            #"-DBUILD_ZLIB=OFF", # Don't use bundled zlib, since we use the version from UE4
            #"-DBUILD_PNG=OFF",   # Don't use bundled libpng, since we use the version from UE4
            #"-DBUILD_JPEG=OFF",

            # Build only the contrib modules we need
            "-DBUILD_opencv_alphamat=OFF",
            "-DBUILD_opencv_aruco=ON",
            "-DBUILD_opencv_barcode=OFF",
            "-DBUILD_opencv_bgsegm=OFF",
            "-DBUILD_opencv_bioinspired=OFF",
            "-DBUILD_opencv_ccalib=OFF",
            "-DBUILD_opencv_cnn_3dobj=OFF",
            "-DBUILD_opencv_cvv=OFF",
            "-DBUILD_opencv_datasets=OFF",
            "-DBUILD_opencv_dnn_objdetect=OFF",
            "-DBUILD_opencv_dnn_superres=OFF",
            "-DBUILD_opencv_dnns_easily_fooled=OFF",
            "-DBUILD_opencv_dpm=OFF",
            "-DBUILD_opencv_face=OFF",
            "-DBUILD_opencv_freetype=OFF",
            "-DBUILD_opencv_fuzzy=OFF",
            "-DBUILD_opencv_hdf=OFF",
            "-DBUILD_opencv_hfs=OFF",
            "-DBUILD_opencv_img_hash=OFF",
            "-DBUILD_opencv_intensity_transform=OFF",
            "-DBUILD_opencv_julia=OFF",
            #"-DBUILD_opencv_legacy=OFF", #idk if this is a real module. It's given as an example for excluding modules in the opencv contrib github repo
            "-DBUILD_opencv_line_descriptor=OFF",
            "-DBUILD_opencv_matlab=OFF",
            "-DBUILD_opencv_mcc=OFF",
            "-DBUILD_opencv_optflow=OFF",
            "-DBUILD_opencv_ovis=OFF",
            "-DBUILD_opencv_phase_unwrapping=OFF",
            "-DBUILD_opencv_plot=OFF",
            "-DBUILD_opencv_quality=OFF",
            "-DBUILD_opencv_rapid=OFF",
            "-DBUILD_opencv_reg=OFF",
            "-DBUILD_opencv_rgbd=OFF",
            "-DBUILD_opencv_saliency=OFF",
            "-DBUILD_opencv_sfm=OFF",
            "-DBUILD_opencv_shape=OFF",
            "-DBUILD_opencv_stereo=OFF",
            "-DBUILD_opencv_structured_light=OFF",
            "-DBUILD_opencv_superres=OFF",
            "-DBUILD_opencv_surface_matching=OFF",
            "-DBUILD_opencv_text=OFF",
            "-DBUILD_opencv_tracking=OFF",
            "-DBUILD_opencv_videostab=OFF",
            "-DBUILD_opencv_viz=OFF",
            "-DBUILD_opencv_wechat_qrcode=OFF",
            "-DBUILD_opencv_xfeatures2d=OFF",
            "-DBUILD_opencv_ximgproc=OFF",
            "-DBUILD_opencv_xobjdetect=OFF",
            "-DBUILD_opencv_xphoto=OFF"
        ]
        
        # Append the flags to ensure OpenCV's FindXXX modules use our UE4-specific dependencies
        #from ue4util import Utility
        from conan_ue4cli.data.packages.ue4util.ue4util import Utility
        #TODO: linking these obviously fails for Android builds if they are not rebuilt for Android, but building them for Android has issues. Try building them the normal OpenCV way first (without linking the Unreal/conan ones) and see if that works. For Linux builds, use the nonstandard Unreal/conan linking.

        #zlib = self.deps_cpp_info["zlib"]
        #libpng = self.deps_cpp_info["UElibPNG"]
        #libjpg = self.deps_cpp_info["libjpeg"]
        ##gstreamer = self.deps_cpp_info["gstreamer"]

        #flags.extend([
        #    "-DPNG_PNG_INCLUDE_DIR=" + libpng.include_paths[0],
        #    "-DPNG_LIBRARY=" + Utility.resolve_file(libpng.lib_paths[0], libpng.libs[0]),
        #    "-DZLIB_INCLUDE_DIR=" + zlib.include_paths[0],
        #    "-DZLIB_LIBRARY=" + Utility.resolve_file(zlib.lib_paths[0], zlib.libs[0])#,
        #    "-DJPEG_INCLUDE_DIR=" + libjpg.include_paths[0],
        #    "-DJPEG_LIBRARY=" + Utility.resolve_file(libjpg.lib_paths[0], libjpg.libs[0])
        #])

        return flags
    
    def source(self):
        self.run("git clone --depth=1 https://github.com/opencv/opencv.git -b {}".format(self.version))
        self.run("git clone --depth=1 https://github.com/opencv/opencv_contrib.git -b {}".format(self.version))
        
        # Disable binary prefixing for installed libs under Windows
        tools.replace_in_file(
            "opencv/CMakeLists.txt",
            '''ocv_update(OPENCV_LIB_INSTALL_PATH   "${OpenCV_INSTALL_BINARIES_PREFIX}staticlib${LIB_SUFFIX}")''',
            '''ocv_update(OPENCV_LIB_INSTALL_PATH   "lib")'''
        )
        
        #Prevent conflicting int64 and uint64 typedefs  - V
        #Needed when doing Linux build, temporarily disabled for Android build attempts
        # tools.replace_in_file(
        #     "opencv/modules/core/include/opencv2/core/hal/interface.h",
        #     '''typedef int64_t int64;''',
        #     '''typedef long long int64;'''
        # )
        # tools.replace_in_file(
        #     "opencv/modules/core/include/opencv2/core/hal/interface.h",
        #     '''typedef uint64_t uint64;''',
        #     '''typedef unsigned long long uint64;'''
        # )

        #Prevent "Detected Apple 'check' macro definition, it can cause build conflicts. Please, include this header before any Apple headers.". This gets treated as an error by the compiler, although it seems to just be a warning in the code, so it might be preventable by decreasing strictness. - V
        #I don't know if this change prevents build failures for iOS builds, and I doubt it does. - V
        #needed fpr Linux (all three replace_in_file calls), disabled for Android attempt
        #tools.replace_in_file(
        #    "opencv/modules/core/include/opencv2/core/utility.hpp",
        #    '''bool check() const;''',
        #    '''bool check_() const;'''
        #)
        #tools.replace_in_file(
        #    "opencv/modules/core/include/opencv2/core/utility.hpp",
        #    '''#  warning Detected Apple 'check' macro definition, it can cause build conflicts. Please, include this header before any Apple headers.''',
        #    '''//#  warning Detected Apple 'check' macro definition, it can cause build conflicts. Please, include this header before any Apple headers.'''
        #)
        #tools.replace_in_file(
        #    "opencv/modules/core/src/command_line_parser.cpp",
        #    '''bool CommandLineParser::check() const''',
        #    '''bool CommandLineParser::check_() const'''
        #)

        
    def build(self):
        
        # Enable compiler interposition under Linux to enforce the correct flags for libc++
        #from libcxx import LibCxx
        from conan_ue4cli.data.packages.libcxx.libcxx import LibCxx
        LibCxx.set_vars(self)
        
        # Build OpenCV
        cmake = CMake(self)
        cmake.configure(source_folder="opencv", args=self.cmake_flags())
        cmake.build()
        cmake.install()
    
    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
