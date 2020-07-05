#include "mesh_data_instance.h"

#if TEXTURE_PACKER_PRESENT
#include "../../texture_packer/texture_resource/packer_image_resource.h"
#endif

#include "core/image.h"

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
	if (_mesh == mesh) {
		return;
	}

	_mesh = mesh;

	if (_mesh.is_valid()) {
		if (is_inside_tree()) {
			setup_mesh();
		} else {
			_dirty = true;
		}
	}
}

Ref<Texture> MeshDataInstance::get_texture() {
	return _texture;
}
void MeshDataInstance::set_texture(const Ref<Texture> &texture) {
	_texture = texture;

	setup_material_texture();
}

Ref<Material> MeshDataInstance::get_material() {
	return _material;
}
void MeshDataInstance::set_material(const Ref<Material> &mat) {
	_material = mat;

	setup_material_texture();
}

void MeshDataInstance::setup_mesh() {
	if (!_mesh.is_valid()) {
		return;
	}

	Array arr = _mesh->get_array();

	Ref<ArrayMesh> mesh = get_mesh();

	if (!mesh.is_valid()) {
		mesh.instance();
	}

	for (int i = 0; i < mesh->get_surface_count(); ++i) {
		mesh->surface_remove(i);
	}

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);

	if (_material.is_valid() && mesh->get_surface_count() > 0) {
		mesh->surface_set_material(0, _material);
	}

	if (get_mesh() == mesh) {
		return;
	}

	set_mesh(mesh);
}

void MeshDataInstance::setup_material_texture() {
	if (!_texture.is_valid()) {
		return;
	}

	if (_material.is_valid()) {
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
			tex->create_from_image(i, 0);

			if (sm.is_valid()) {
				sm->set_texture(SpatialMaterial::TEXTURE_ALBEDO, tex);
			}

			return;
		}
#endif

		sm->set_texture(SpatialMaterial::TEXTURE_ALBEDO, _texture);
	}
}

MeshDataInstance::MeshDataInstance() {
	_dirty = false;
	_snap_to_mesh = false;
	_snap_axis = Vector3(0, -1, 0);
}
MeshDataInstance::~MeshDataInstance() {
	_mesh.unref();
	_texture.unref();
}

void MeshDataInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (_dirty) {
				_dirty = false;

				if (_mesh.is_valid()) {
					setup_mesh();
				}
			}
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
}
