// Vendored third-party implementation unit, isolated so its warnings never
// mix with ours (ui-merlin/ and sim/ build clean under -Wall -Wextra).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma clang diagnostic pop
