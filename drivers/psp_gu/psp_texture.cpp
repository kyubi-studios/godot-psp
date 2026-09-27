#include "psp_texture.h"

#include <malloc.h>
#include <pspgu.h>
#include <pspkernel.h>

// En yakın 2'nin kuvveti (eşitlikte yukarı), [p_min, p_max] aralığında.
static int _nearest_pot(int p_v, int p_min, int p_max) {
	int lo = 1;
	while (lo * 2 <= p_v) {
		lo *= 2;
	}
	int hi = lo * 2;
	int r = (p_v - lo < hi - p_v) ? lo : hi;
	if (p_v == lo) {
		r = lo;
	}
	return CLAMP(r, p_min, p_max);
}

void PSPTexture::swizzle(uint8_t *r_out, const uint8_t *p_in, uint32_t p_width_bytes, uint32_t p_height) {
	// 16 byte x 8 satırlık bloklar (pspsdk örneklerindeki düzen).
	const uint32_t width_blocks = p_width_bytes / 16;
	const uint32_t height_blocks = p_height / 8;
	const uint32_t src_pitch = (p_width_bytes - 16) / 4;
	const uint32_t src_row = p_width_bytes * 8;
	const uint8_t *ysrc = p_in;
	uint32_t *dst = (uint32_t *)r_out;
	for (uint32_t by = 0; by < height_blocks; by++) {
		const uint8_t *xsrc = ysrc;
		for (uint32_t bx = 0; bx < width_blocks; bx++) {
			const uint32_t *src = (const uint32_t *)xsrc;
			for (int j = 0; j < 8; j++) {
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}

bool PSPTexture::convert(const Ref<Image> &p_image, PSPTextureData &r_data) {
	r_data = PSPTextureData();
	ERR_FAIL_COND_V(p_image.is_null() || p_image->is_empty(), false);

	const int sw = p_image->get_width();
	const int sh = p_image->get_height();
	const int w = _nearest_pot(sw, MIN_WIDTH, MAX_SIZE);
	const int h = _nearest_pot(sh, 1, MAX_SIZE);

	// Bellek: tüm görüntüyü kopyalayıp açmak yerine hedefe yeten en küçük mip seviyesini ayır.
	Ref<Image> img;
	if (p_image->has_mipmaps()) {
		int level = 0;
		for (int l = 1; l <= p_image->get_mipmap_count(); l++) {
			int64_t ofs, size;
			int lw, lh;
			p_image->get_mipmap_offset_size_and_dimensions(l, ofs, size, lw, lh);
			if (lw < w || lh < h) {
				break;
			}
			level = l;
		}
		int64_t ofs, size;
		int lw, lh;
		p_image->get_mipmap_offset_size_and_dimensions(level, ofs, size, lw, lh);
		img = Image::create_from_data(lw, lh, false, p_image->get_format(), p_image->get_data().slice(ofs, ofs + size));
	} else {
		img = p_image->duplicate();
	}
	if (img->is_compressed() && img->decompress() != OK) {
		WARN_PRINT_ONCE("PSP: compressed texture format cannot be decoded; use 'VRAM Uncompressed' or 'Lossless' import.");
		return false;
	}
	img->clear_mipmaps();
	if (img->get_format() != Image::FORMAT_RGBA8) {
		img->convert(Image::FORMAT_RGBA8);
	}
	if (w != img->get_width() || h != img->get_height()) {
		const bool upscale = w >= img->get_width() && h >= img->get_height();
		img->resize(w, h, upscale ? Image::INTERPOLATE_NEAREST : Image::INTERPOLATE_BILINEAR);
	}

	const uint8_t *src = img->ptr();
	const int count = w * h;
	bool has_alpha = false, soft_alpha = false;
	for (int i = 0; i < count; i++) {
		const uint8_t a = src[i * 4 + 3];
		if (a != 255) {
			has_alpha = true;
			if (a != 0) {
				soft_alpha = true;
				break;
			}
		}
	}
	const int psm = !has_alpha ? GU_PSM_5650 : (soft_alpha ? GU_PSM_4444 : GU_PSM_5551);

	const uint32_t bytes = count * 2;
	uint16_t *lin = (uint16_t *)memalign(16, bytes);
	ERR_FAIL_NULL_V(lin, false);
	for (int i = 0; i < count; i++) {
		const uint8_t *p = src + i * 4;
		uint16_t v;
		if (psm == GU_PSM_5650) {
			v = (p[0] >> 3) | ((p[1] >> 2) << 5) | ((p[2] >> 3) << 11);
		} else if (psm == GU_PSM_5551) {
			v = (p[0] >> 3) | ((p[1] >> 3) << 5) | ((p[2] >> 3) << 10) | ((p[3] >= 128) << 15);
		} else {
			v = (p[0] >> 4) | ((p[1] >> 4) << 4) | ((p[2] >> 4) << 8) | ((p[3] >> 4) << 12);
		}
		lin[i] = v;
	}
	img.unref(); // RGBA8 kopyayı hemen bırak

	void *out = lin;
	bool swizzled = false;
	if (h >= 8) {
		uint8_t *swz = (uint8_t *)memalign(16, bytes);
		if (swz) {
			swizzle(swz, (const uint8_t *)lin, w * 2, h);
			free(lin);
			out = swz;
			swizzled = true;
		}
	}
	sceKernelDcacheWritebackRange(out, bytes);

	r_data.pixels = out;
	r_data.psm = psm;
	r_data.width = w;
	r_data.height = h;
	r_data.source_width = sw;
	r_data.source_height = sh;
	r_data.bytes = bytes;
	r_data.swizzled = swizzled;
	return true;
}

void PSPTexture::free_data(PSPTextureData &r_data) {
	if (r_data.pixels) {
		free(r_data.pixels);
	}
	r_data = PSPTextureData();
}
