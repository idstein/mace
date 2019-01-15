cc_library(
    name = "tflite",
    hdrs = [
        "common.h",
        "compatibility.h",
        "optimized/depthwiseconv_uint8_3x3_filter.h",
        "optimized/depthwiseconv_uint8.h",
        "types.h",
    ],
    include_prefix = "tensorflow/lite/kernels/internal",
    deps = [
        "@arm_neon_2_x86_sse",
        "@tflite_kernels",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
)
