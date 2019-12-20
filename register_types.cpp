#include "register_types.h"

#include "mesh_data_resource.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_plugin.h"

#include "editor_plugin_collada_mdr.h"
#endif


void register_mesh_data_resource_types() {
	ClassDB::register_class<MeshDataResource>();

	#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<EditorPluginColladaMdr>();
	#endif
}

void unregister_mesh_data_resource_types() {

}

