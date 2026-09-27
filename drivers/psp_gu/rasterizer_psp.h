#pragma once

#include "servers/rendering/dummy/rasterizer_dummy.h"

// PSP GE renderer. RasterizerDummy'den türer: fog/GI/particles/canvas dummy kalır,
// sahne ve depolama sınıfları PSP sürümleriyle değiştirilir.
class RasterizerPSP : public RasterizerDummy {
public:
	void initialize() override;
	void begin_frame(double p_frame_step) override;
	void end_frame(bool p_present) override;

	static RendererCompositor *_create_current() {
		return memnew(RasterizerPSP);
	}
	static void make_current() {
		_create_func = _create_current;
		low_end = true;
	}

	RasterizerPSP();
};
