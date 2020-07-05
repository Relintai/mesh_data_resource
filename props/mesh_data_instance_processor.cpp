#include "mesh_data_instance_processor.h"

#if PROPS_PRESENT

#include "../nodes/mesh_data_instance.h"
#include "prop_data_mesh.h"

bool MeshDataInstanceProcessor::_handles(Node *node) {
	MeshDataInstance *i = Object::cast_to<MeshDataInstance>(node);

	return i;
}

void MeshDataInstanceProcessor::_process(Ref<PropData> prop_data, Node *node, const Transform &transform) {
	MeshDataInstance *i = Object::cast_to<MeshDataInstance>(node);

	ERR_FAIL_COND(!i);

	Ref<PropDataMesh> m;
	m.instance();
	m->set_mesh(i->get_mesh_data());
	m->set_texture(i->get_texture());
	m->set_transform(transform * i->get_transform());
	prop_data->add_prop(m);
}

MeshDataInstanceProcessor::MeshDataInstanceProcessor() {
}
MeshDataInstanceProcessor::~MeshDataInstanceProcessor() {
}

void MeshDataInstanceProcessor::_bind_methods() {
}

#endif