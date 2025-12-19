bl_info = {
    "name": "Game Animation FBX Exporter (Active Action)",
    "author": "ChatGPT",
    "version": (1, 2, 0),
    "blender": (3, 6, 0),
    "location": "File > Export > Game Animation FBX (Active)",
    "description": "Exports ONLY the active Action as game-ready FBX",
    "category": "Import-Export",
}

import bpy
import os


class EXPORT_OT_game_anim_fbx(bpy.types.Operator):
    bl_idname = "export_scene.game_anim_fbx_active"
    bl_label = "Export Game Animation FBX (Active Action)"
    bl_options = {'REGISTER', 'UNDO'}

    filepath: bpy.props.StringProperty(
        name="File Path",
        subtype='FILE_PATH'
    )

    scale: bpy.props.FloatProperty(
        name="Scale",
        default=1.0
    )

    def execute(self, context):
        armature = None

        for obj in context.selected_objects:
            if obj.type == 'ARMATURE':
                armature = obj
                break

        if not armature:
            self.report({'ERROR'}, "Select an Armature")
            return {'CANCELLED'}

        if not armature.animation_data or not armature.animation_data.action:
            self.report({'ERROR'}, "No active Action on Armature")
            return {'CANCELLED'}

        action = armature.animation_data.action
        armature.animation_data.action = action

        bpy.ops.object.mode_set(mode='OBJECT')

        bpy.ops.export_scene.fbx(
        filepath=self.filepath,
        use_selection=True,
        object_types={'ARMATURE', 'MESH'},
        use_mesh_modifiers=True,

        global_scale=self.scale,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL',

        axis_forward='Z',
        axis_up='Y',

        add_leaf_bones=False,
        use_armature_deform_only=True,   # ✅ FIX
        primary_bone_axis='Y',
        secondary_bone_axis='X',

        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=1.0,

        path_mode='AUTO',
        embed_textures=False
    )

        self.report({'INFO'}, f"Exported: {action.name}")
        return {'FINISHED'}

    def invoke(self, context, event):
        for obj in context.selected_objects:
            if obj.type == 'ARMATURE' and obj.animation_data and obj.animation_data.action:
                action_name = obj.animation_data.action.name
                self.filepath = bpy.path.ensure_ext(action_name, ".fbx")
                break

        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


def menu_func_export(self, context):
    self.layout.operator(
        EXPORT_OT_game_anim_fbx.bl_idname,
        text="Game Animation FBX (Active Action)"
    )


def register():
    bpy.utils.register_class(EXPORT_OT_game_anim_fbx)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    bpy.utils.unregister_class(EXPORT_OT_game_anim_fbx)


if __name__ == "__main__":
    register()
