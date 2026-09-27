#pragma once

#include "servers/rendering/dummy/storage/utilities.h"

namespace RendererPSP {

// RID türünü ve serbest bırakmayı PSP depolarına yönlendirir; bilinmeyenler dummy'ye düşer.
class Utilities : public RendererDummy::Utilities {
public:
	RSE::InstanceType get_base_type(RID p_rid) const override;
	bool free(RID p_rid) override;
	void base_update_dependency(RID p_base, DependencyTracker *p_instance) override;
	bool has_os_feature(const String &p_feature) const override { return false; }
	Size2i get_maximum_viewport_size() const override { return Size2i(480, 272); }
};

} // namespace RendererPSP
