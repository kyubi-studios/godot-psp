#pragma once

#include "core/templates/rid_owner.h"
#include "servers/rendering/dummy/storage/texture_storage.h"

namespace RendererPSP {

// Render target'lar: PSP'de tek ekran framebuffer'ı vardır; Godot viewport'ları geçerli bir render
// target ister (yoksa çizimi atlar). Boyut ve temizleme isteği tutulur, çizim doğrudan ekrana gider.
class TextureStorage : public RendererDummy::TextureStorage {
	struct RenderTarget {
		Size2i size;
		bool clear_requested = false;
		Color clear_color;
		bool direct_to_screen = true;
	};
	mutable RID_Owner<RenderTarget> render_target_owner;

public:
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
