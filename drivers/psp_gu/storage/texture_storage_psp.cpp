#include "texture_storage_psp.h"

#include "drivers/psp_gu/psp_gu.h"

using namespace RendererPSP;

/* TEXTURES */

RID TextureStorage::texture_allocate() {
	return texture_owner.allocate_rid();
}

void TextureStorage::texture_free(RID p_rid) {
	Texture *t = texture_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(t);
	total_bytes -= t->data.bytes;
	PSPTexture::free_data(t->data);
	texture_owner.free(p_rid);
}

void TextureStorage::texture_2d_initialize(RID p_texture, const Ref<Image> &p_image) {
	Texture t;
	if (p_image.is_valid()) {
		t.format = p_image->get_format();
		if (!PSPTexture::convert(p_image, t.data)) {
			WARN_PRINT_ONCE("PSP: a texture could not be converted and will be drawn untextured.");
		}
	}
	total_bytes += t.data.bytes;
	texture_owner.initialize_rid(p_texture, t);
}

void TextureStorage::texture_2d_update(RID p_texture, const Ref<Image> &p_image, int p_layer) {
	Texture *t = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(t);
	total_bytes -= t->data.bytes;
	PSPTexture::free_data(t->data);
	if (p_image.is_valid()) {
		t->format = p_image->get_format();
		if (!PSPTexture::convert(p_image, t->data)) {
			WARN_PRINT_ONCE("PSP: a texture update could not be converted; the texture is drawn untextured.");
		}
	}
	total_bytes += t->data.bytes;
}

void TextureStorage::texture_2d_placeholder_initialize(RID p_texture) {
	_init_empty(p_texture);
}

void TextureStorage::texture_replace(RID p_texture, RID p_by_texture) {
	// p_by_texture'ın verisi p_texture'a taşınır, p_by_texture silinir (RID'ler geçerli kalır).
	Texture *t = texture_owner.get_or_null(p_texture);
	Texture *by = texture_owner.get_or_null(p_by_texture);
	ERR_FAIL_NULL(t);
	ERR_FAIL_NULL(by);
	total_bytes -= t->data.bytes;
	PSPTexture::free_data(t->data);
	*t = *by;
	by->data = PSPTextureData(); // sahiplik devredildi
	texture_owner.free(p_by_texture);
}

Image::Format TextureStorage::texture_get_format(RID p_texture) const {
	Texture *t = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(t, Image::FORMAT_MAX);
	return t->format;
}

Size2 TextureStorage::texture_size_with_proxy(RID p_proxy) {
	Texture *t = texture_owner.get_or_null(p_proxy);
	ERR_FAIL_NULL_V(t, Size2());
	return Size2(t->data.source_width, t->data.source_height);
}

/* RENDER TARGETS */

RID TextureStorage::render_target_create() {
	return render_target_owner.make_rid(RenderTarget());
}

void TextureStorage::render_target_free(RID p_rid) {
	if (render_target_owner.owns(p_rid)) {
		render_target_owner.free(p_rid);
	}
}

void TextureStorage::render_target_set_size(RID p_render_target, int p_width, int p_height, uint32_t p_view_count) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->size = Size2i(p_width, p_height);
}

Size2i TextureStorage::render_target_get_size(RID p_render_target) const {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Size2i());
	return rt->size;
}

void TextureStorage::render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->direct_to_screen = p_direct_to_screen;
}

bool TextureStorage::render_target_get_direct_to_screen(RID p_render_target) const {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);
	return rt->direct_to_screen;
}

void TextureStorage::render_target_request_clear(RID p_render_target, const Color &p_clear_color) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->clear_requested = true;
	rt->clear_color = p_clear_color;
}

bool TextureStorage::render_target_is_clear_requested(RID p_render_target) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);
	return rt->clear_requested;
}

Color TextureStorage::render_target_get_clear_request_color(RID p_render_target) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Color());
	return rt->clear_color;
}

void TextureStorage::render_target_disable_clear_request(RID p_render_target) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->clear_requested = false;
}

void TextureStorage::render_target_do_clear_request(RID p_render_target) {
	RenderTarget *rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (rt->clear_requested && PSPGU::in_frame()) {
		const Color &c = rt->clear_color;
		PSPGU::clear(PSPGU::color_to_abgr(c.r, c.g, c.b, 1.0f));
	}
	rt->clear_requested = false;
}
