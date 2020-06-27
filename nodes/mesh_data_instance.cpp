#include "mesh_data_instance.h"

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

Ref<MeshDataResource> MeshDataInstance::get_mesh() {
	return _mesh;
}
void MeshDataInstance::set_mesh(const Ref<MeshDataResource> &mesh) {
	_mesh = mesh;
}

Ref<Texture> MeshDataInstance::get_texture() {
	return _texture;
}
void MeshDataInstance::set_texture(const Ref<Texture> &texture) {
	_texture = texture;
}

MeshDataInstance::MeshDataInstance() {
	_snap_to_mesh = false;
	_snap_axis = Vector3(0, -1, 0);
}
MeshDataInstance::~MeshDataInstance() {
	_mesh.unref();
	_texture.unref();
}

void MeshDataInstance::notification(int p_what) {
	/*
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (get_parent() == NULL)
				return;
		}
		//fallthrough
		case NOTIFICATION_PARENTED: {
			Node *n = this;

			while (n) {

				PropInstance *pi = Object::cast_to<PropInstance>(n);

				if (pi) {
					_owner = pi;
					pi->register_prop_mesh_data_instance(this);
					return;
				}

				n = n->get_parent();
			}

		} break;
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_UNPARENTED: {
			if (_owner) {
				_owner->unregister_prop_mesh_data_instance(this);
			}
			break;
		}
	}*/
}

void MeshDataInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_snap_to_mesh"), &MeshDataInstance::get_snap_to_mesh);
	ClassDB::bind_method(D_METHOD("set_snap_to_mesh", "value"), &MeshDataInstance::set_snap_to_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "snap_to_mesh"), "set_snap_to_mesh", "get_snap_to_mesh");

	ClassDB::bind_method(D_METHOD("get_snap_axis"), &MeshDataInstance::get_snap_axis);
	ClassDB::bind_method(D_METHOD("set_snap_axis", "value"), &MeshDataInstance::set_snap_axis);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "snap_axis"), "set_snap_axis", "get_snap_axis");

	ClassDB::bind_method(D_METHOD("get_mesh"), &MeshDataInstance::get_mesh);
	ClassDB::bind_method(D_METHOD("set_mesh", "value"), &MeshDataInstance::set_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "MeshDataResource"), "set_mesh", "get_mesh");

	ClassDB::bind_method(D_METHOD("get_texture"), &MeshDataInstance::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture", "value"), &MeshDataInstance::set_texture);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_texture", "get_texture");
}
