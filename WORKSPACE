workspace(name = "mace")

# generate version and opencl kernel code.
load("//repository/git:git_configure.bzl", "git_version_repository")
load("//repository/opencl-kernel:opencl_kernel_configure.bzl", "encrypt_opencl_kernel_repository")

git_version_repository(name = "local_version_config")

encrypt_opencl_kernel_repository(name = "local_opencl_kernel_encrypt")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
# proto_library rules implicitly depend on @com_google_protobuf//:protoc,
# which is the proto-compiler.
# This statement defines the @com_google_protobuf repo.
http_archive(
    name = "com_google_protobuf",
    sha256 = "9510dd2afc29e7245e9e884336f848c8a6600a14ae726adb6befdb4f786f0be2",
    strip_prefix = "protobuf-3.6.1.3",
    urls = [
        "https://github.com/google/protobuf/archive/v3.6.1.3.zip",
    ],
)

http_archive(
    name = "gtest",
    build_file = "//:third_party/googletest/googletest.BUILD",
    sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
    strip_prefix = "googletest-release-1.8.0",
    urls = [
        "https://github.com/google/googletest/archive/release-1.8.0.zip",
    ],
)

http_archive(
    name = "opencl_headers",
    build_file = "//:third_party/opencl-headers/opencl-headers.BUILD",
    sha256 = "b2b813dd88a7c39eb396afc153070f8f262504a7f956505b2049e223cfc2229b",
    strip_prefix = "OpenCL-Headers-f039db6764d52388658ef15c30b2237bbda49803",
    urls = [
        "https://github.com/KhronosGroup/OpenCL-Headers/archive/f039db6764d52388658ef15c30b2237bbda49803.zip",
    ],
)

http_archive(
    name = "opencl_clhpp",
    build_file = "//:third_party/opencl-clhpp/opencl-clhpp.BUILD",
    sha256 = "dab6f1834ec6e3843438cc0f97d63817902aadd04566418c1fcc7fb78987d4e7",
    strip_prefix = "OpenCL-CLHPP-4c6f7d56271727e37fb19a9b47649dd175df2b12",
    urls = [
        "https://github.com/KhronosGroup/OpenCL-CLHPP/archive/4c6f7d56271727e37fb19a9b47649dd175df2b12.zip",
    ],
)

http_archive(
    name = "half",
    build_file = "//:third_party/half/half.BUILD",
    sha256 = "2a8f0b8fd403392b182b0409e96556a84e2cede8e7ce71810dda52b3eed5e526",
    strip_prefix = "half-code-r361-trunk",
    urls = [
        "https://sourceforge.net/code-snapshots/svn/h/ha/half/code/half-code-r361-trunk.zip",
    ],
)

http_archive(
    name = "eigen",
    build_file = "//:third_party/eigen3/eigen.BUILD",
    sha256 = "ca7beac153d4059c02c8fc59816c82d54ea47fe58365e8aded4082ded0b820c4",
    strip_prefix = "eigen-eigen-f3a22f35b044",
    urls = [
        "http://mirror.bazel.build/bitbucket.org/eigen/eigen/get/f3a22f35b044.tar.gz",
        "https://bitbucket.org/eigen/eigen/get/f3a22f35b044.tar.gz",
    ],
)

http_archive(
    name = "gemmlowp",
    sha256 = "614f59c80e7e05bd14a9d1a8c0716d9aca6d393d82ffd4ca96926cb8ae4fe19a",
    strip_prefix = "gemmlowp-2233f5e6ae1aead4f0fa951f84772737f07dd291",
    urls = [
        "https://github.com/google/gemmlowp/archive/2233f5e6ae1aead4f0fa951f84772737f07dd291.zip",
    ],
)

http_archive(
    name = "tflite",
    sha256 = "c886d46ad8c91fcafed2d910ad9e7bc5aeb29856c387bdf9b6b4903cc16e6e60",
    strip_prefix = "tensorflow-mace-ffc8cc7e8c9d1894753509e88b17e251bc6255e3",
    urls = [
        "https://cnbj1.fds.api.xiaomi.com/mace/third-party/tflite/tensorflow-mace-ffc8cc7e8c9d1894753509e88b17e251bc6255e3.zip",
    ],
)

http_archive(
    name = "six_archive",
    build_file = "//:third_party/six/six.BUILD",
    sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
    strip_prefix = "six-1.10.0",
    urls = [
        "http://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
        "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
    ],
)

bind(
    name = "six",
    actual = "@six_archive//:six",
)

http_archive(
    # v2.2.0 + fix of include path
    name = "com_github_gflags_gflags",
    sha256 = "16903f6bb63c00689eee3bf7fb4b8f242934f6c839ce3afc5690f71b712187f9",
    strip_prefix = "gflags-30dbc81fb5ffdc98ea9b14b1918bfe4e8779b26e",
    urls = [
        "https://github.com/gflags/gflags/archive/30dbc81fb5ffdc98ea9b14b1918bfe4e8779b26e.zip",
    ],
)

bind(
    name = "gflags",
    actual = "@com_github_gflags_gflags//:gflags",
)

bind(
    name = "gflags_nothreads",
    actual = "@com_github_gflags_gflags//:gflags_nothreads",
)

# Set up Android NDK
android_ndk_repository(
    name = "androidndk",
    # Android 5.0
    api_level = 21,
)

# Set up default cross compilers for arm linux
http_archive(
    name = "gcc_linaro_7_3_1_arm_linux_gnueabihf",
    build_file = "//:third_party/compilers/arm_compiler.BUILD",
    sha256 = "7248bf105d0d468887a9b8a7120bb281ac8ad0223d9cb3d00dc7c2d498485d91",
    strip_prefix = "gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf",
    urls = [
        "https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/arm-linux-gnueabihf/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz",
    ],
)

http_archive(
    name = "gcc_linaro_7_3_1_aarch64_linux_gnu",
    build_file = "//:third_party/compilers/aarch64_compiler.BUILD",
    sha256 = "73eed74e593e2267504efbcf3678918bb22409ab7afa3dc7c135d2c6790c2345",
    strip_prefix = "gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu",
    urls = [
        "https://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz",
    ],
)
