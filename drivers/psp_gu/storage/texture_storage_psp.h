#pragma once

#include "core/templates/rid_owner.h"
#include "drivers/psp_gu/psp_texture.h"
#include "servers/rendering/dummy/storage/texture_storage.h"

namespace RendererPSP {

// Render target'lar: PSP'de tek ekran framebuffer'ı vardır; Godot viewport'ları geçerli bir render
// target ister (yoksa çizimi atlar). Boyut ve temizleme isteği tutulur, çizim doğrudan ekrana gider.
class TextureStorage : public RendererDummy::TextureStorage {
public:
	struct Texture {
		PSPTextureData data; // GE'ye hazır piksel verisi (orijinal Image saklanmaz)
		Image::Format format = Image::FORMAT_RGBA8;
	};

private:
	mutable RID_Owner<Texture, true> texture_owner;
	uint32_t total_bytes = 0;

	void _init_empty(RID p_texture) { texture_owner.initialize_rid(p_texture, Texture()); }

	struct RenderTarget {
		Size2i size;
		bool clear_requested = false;
		Color clear_color;
		bool direct_to_screen = true;
	};
	mutable RID_Owner<RenderTarget> render_target_owner;

public:
	bool owns_psp_texture(RID p_rid) const { return texture_owner.owns(p_rid); }
	const Texture *get_psp_texture(RID p_rid) const { return texture_owner.get_or_null(p_rid); }
	uint32_t get_total_bytes() const { return total_bytes; }

	RID texture_allocate() override;
	void texture_free(RID p_rid) override;
	void texture_2d_initialize(RID p_texture, const Ref<Image> &p_image) override;
	void texture_2d_update(RID p_texture, const Ref<Image> &p_image, int p_layer = 0) override;
	void texture_2d_placeholder_initialize(RID p_texture) override;
	// Desteklenmeyen türler: RID yine de başlatılır (boş texture), yoksa texture_free hata verir.
	void texture_2d_layered_initialize(RID p_texture, const Vector<Ref<Image>> &p_layers, RSE::TextureLayeredType p_layered_type) override { _init_empty(p_texture); }
	void texture_3d_initialize(RID p_texture, Image::Format, int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>> &p_data) override { _init_empty(p_texture); }
	void texture_external_initialize(RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) override { _init_empty(p_texture); }
	void texture_proxy_initialize(RID p_texture, RID p_base) override { _init_empty(p_texture); }
	void texture_drawable_initialize(RID p_texture, int p_width, int p_height, RSE::TextureDrawableFormat p_format, const Color &p_color, bool p_with_mipmaps) override { _init_empty(p_texture); }
	void texture_2d_layered_placeholder_initialize(RID p_texture, RSE::TextureLayeredType p_layered_type) override { _init_empty(p_texture); }
	void texture_3d_placeholder_initialize(RID p_texture) override { _init_empty(p_texture); }
	Ref<Image> texture_2d_get(RID p_texture) const override { return Ref<Image>(); }
	void texture_replace(RID p_texture, RID p_by_texture) override;
	Image::Format texture_get_format(RID p_texture) const override;
	Size2 texture_size_with_proxy(RID p_proxy) override;

	RID render_target_create() override;
	void render_target_free(RID p_rid) override;
	void render_target_set_size(RID p_render_target, int p_width, int p_height, uint32_t p_view_count) override;
	Size2i render_target_get_size(RID p_render_target) const override;
	void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen) override;
	bool render_target_get_direct_to_screen(RID p_render_target) const override;
	bool render_target_was_used(RID p_render_target) const override { return true; }
	void render_target_request_clear(RID p_render_target, const Color &p_clear_color) override;
	bool render_target_is_clear_requested(RID p_render_target) override;
	Color render_target_get_clear_request_color(RID p_render_target) override;
	void render_target_disable_clear_request(RID p_render_target) override;
	void render_target_do_clear_request(RID p_render_target) override;

	bool owns_render_target(RID p_rid) const { return render_target_owner.owns(p_rid); }
};

} // namespace RendererPSP
