#include "mesh_data_instance.h"

#include "core/version.h"

#include "core/version.h"
#include "scene/resources/texture.h"

#if VERSION_MAJOR < 4
#include "core/image.h"

#define GET_WORLD get_world
#else
#include "core/io/image.h"

#define GET_WORLD get_world_3d
#endif

#if TEXTURE_PACKER_PRESENT
#include "../../texture_packer/texture_resource/packer_image_resource.h"
#endif

bool MeshDataInstance::get_snap_to_mesh() const {
	return _snap_to_mesh;
}
void MeshDataInstance::set_snap_to_mesh(const bool value) {
	_snap_to_mesh = value;
}

Vector3 MeshDataInstance::get_snap_axis() const {
	return _snap_axis;
}
void MeshDataInstance::set_snap_axis(const Vector3 &value) {
	_snap_axis = value;
}

Ref<MeshDataResource> MeshDataInstance::get_mesh_data() {
	return _mesh;
}
void MeshDataInstance::set_mesh_data(const Ref<MeshDataResource> &mesh) {
	_mesh = mesh;

	refresh();
}

Ref<Texture> MeshDataInstance::get_texture() {
	return _texture;
}
void MeshDataInstance::set_texture(const Ref<Texture> &texture) {
	_texture = texture;

	setup_material_texture();
	refresh();
}

Ref<Material> MeshDataInstance::get_material() {
	return _material;
}
void MeshDataInstance::set_material(const Ref<Material> &mat) {
	_material = mat;

	setup_material_texture();
	refresh();
}

void MeshDataInstance::refresh() {
	if (!is_inside_tree()) {
		return;
	}

	if (_mesh_rid == RID()) {
		_mesh_instance = VisualServer::get_singleton()->instance_create();

		if (GET_WORLD().is_valid())
			VS::get_singleton()->instance_set_scenario(_mesh_instance, GET_WORLD()->get_scenario());

		_mesh_rid = VisualServer::get_singleton()->mesh_create();

		VS::get_singleton()->instance_set_base(_mesh_instance, _mesh_rid);
		VS::get_singleton()->instance_set_transform(_mesh_instance, get_transform());

		VS::get_singleton()->instance_set_visible(_mesh_instance, true);
	}

	VisualServer::get_singleton()->mesh_clear(_mesh_rid);

	if (!_mesh.is_valid()) {
		return;
	}

	Array arr = _mesh->get_array();

	if (arr.size() != Mesh::ARRAY_MAX) {
		return;
	}

	PoolVector<Vector3> vertices = arr[Mesh::ARRAY_VERTEX];

	if (vertices.size() == 0) {
		return;
	}

	VisualServer::get_singleton()->mesh_add_surface_from_arrays(_mesh_rid, VisualServer::PRIMITIVE_TRIANGLES, arr);

	if (_material.is_valid()) {
		VisualServer::get_singleton()->mesh_surface_set_material(_mesh_rid, 0, _material->get_rid());
	}
}

void MeshDataInstance::setup_material_texture() {
	if (!is_inside_tree()) {
		return;
	}

	if (!_texture.is_valid()) {
		if (_material.is_valid()) {
			Ref<SpatialMaterial> sm = _material;

			if (!sm.is_valid()) {
				return;
			}

			sm->set_texture(SpatialMaterial::TEXTURE_ALBEDO, _texture);
		}

		return;
	} else {
		Ref<SpatialMaterial> sm = _material;

		if (!sm.is_valid()) {
			return;
		}

#if TEXTURE_PACKER_PRESENT
		Ref<PackerImageResource> r = _texture;

		if (r.is_valid()) {
			Ref<Image> i = r->get_data();

			Ref<ImageTexture> tex;
			tex.instance();
#if VERSION_MAJOR < 4
			tex->create_from_image(i, 0);
#else
			tex->create_from_image(i);
#endif

			if (sm.is_valid()) {
				sm->set_texture(SpatialMaterial::TEXTURE_ALBEDO, tex);
			}

			return;
		}
#endif

		sm->set_texture(SpatialMaterial::TEXTURE_ALBEDO, _texture);
	}
}

void MeshDataInstance::free_meshes() {
	if (_mesh_instance != RID()) {
		VS::get_singleton()->free(_mesh_instance);
		_mesh_instance = RID();
	}

	if (_mesh_rid != RID()) {
		VS::get_singleton()->free(_mesh_rid);
		_mesh_rid = RID();
	}
}

MeshDataInstance::MeshDataInstance() {
	_dirty = false;
	_snap_to_mesh = false;
	_snap_axis = Vector3(0, -1, 0);

	set_notify_transform(true);
}
MeshDataInstance::~MeshDataInstance() {
	_mesh.unref();
	_texture.unref();
}

void MeshDataInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			setup_material_texture();
			refresh();

			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			free_meshes();
			break;
		}
		case NOTIFICATION_TRANSFORM_CHANGED: {
			VisualServer *vs = VisualServer::get_singleton();

			if (_mesh_instance != RID()) {
				vs->instance_set_transform(_mesh_instance, get_global_transform());
			}

			break;
		}
	}
}

void MeshDataInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_snap_to_mesh"), &MeshDataInstance::get_snap_to_mesh);
	ClassDB::bind_method(D_METHOD("set_snap_to_mesh", "value"), &MeshDataInstance::set_snap_to_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "snap_to_mesh"), "set_snap_to_mesh", "get_snap_to_mesh");

	ClassDB::bind_method(D_METHOD("get_snap_axis"), &MeshDataInstance::get_snap_axis);
	ClassDB::bind_method(D_METHOD("set_snap_axis", "value"), &MeshDataInstance::set_snap_axis);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "snap_axis"), "set_snap_axis", "get_snap_axis");

	ClassDB::bind_method(D_METHOD("get_mesh_data"), &MeshDataInstance::get_mesh_data);
	ClassDB::bind_method(D_METHOD("set_mesh_data", "value"), &MeshDataInstance::set_mesh_data);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh_data", PROPERTY_HINT_RESOURCE_TYPE, "MeshDataResource"), "set_mesh_data", "get_mesh_data");

	ClassDB::bind_method(D_METHOD("get_texture"), &MeshDataInstance::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture", "value"), &MeshDataInstance::set_texture);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_texture", "get_texture");

	ClassDB::bind_method(D_METHOD("get_material"), &MeshDataInstance::get_material);
	ClassDB::bind_method(D_METHOD("set_material", "value"), &MeshDataInstance::set_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");

	ClassDB::bind_method(D_METHOD("refresh"), &MeshDataInstance::refresh);
}
