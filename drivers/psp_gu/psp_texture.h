#pragma once

#include "core/io/image.h"

#include <cstdint>

// Godot Image → GE texture (ana RAM, 16 byte hizalı, swizzle'lı, 16-bit).
struct PSPTextureData {
	void *pixels = nullptr; // memalign(16); free() ile bırakılır
	int psm = 0; // GU_PSM_5650 / GU_PSM_5551 / GU_PSM_4444
	int width = 0; // en yakın 2'nin kuvveti, 8..256
	int height = 0; // en yakın 2'nin kuvveti, 1..256
	int source_width = 0;
	int source_height = 0;
	uint32_t bytes = 0;
	bool swizzled = false; // yükseklik < 8 ise doğrusal kalır
};

namespace PSPTexture {

constexpr int MAX_SIZE = 256;
constexpr int MIN_WIDTH = 8; // 16-bit swizzle bloğu 16 byte genişliğinde

// Dönüştürür; r_data.pixels'i doldurur. Başarısızlıkta false (r_data boş kalır).
bool convert(const Ref<Image> &p_image, PSPTextureData &r_data);
void free_data(PSPTextureData &r_data);

// Test için: doğrusal 16-bit pikselleri GE swizzle düzenine çevirir (width_bytes >= 16, 16'nın katı; height 8'in katı değilse son blok kısmi).
void swizzle(uint8_t *r_out, const uint8_t *p_in, uint32_t p_width_bytes, uint32_t p_height);

} // namespace PSPTexture
