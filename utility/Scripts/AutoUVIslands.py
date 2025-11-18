import bpy
import bmesh

TARGET_SIZE = 1/40  # UV size for 32x32 tile

print("Starting V7 Auto-Tile UV Processor")

orig_mode = bpy.context.object.mode
objects = [o for o in bpy.context.selected_objects if o.type == 'MESH']

def get_connected_vertex_groups(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()

    visited = set()
    groups = []

    for v in bm.verts:
        if v.index in visited:
            continue

        stack = [v]
        group = set()

        while stack:
            cur = stack.pop()
            if cur.index in visited:
                continue

            visited.add(cur.index)
            group.add(cur.index)

            for e in cur.link_edges:
                other = e.other_vert(cur)
                if other.index not in visited:
                    stack.append(other)

        groups.append(group)

    bm.free()
    return groups

def collect_uv_islands(bm, uv_layer):
    islands = []
    visited = set()

    for face in bm.faces:
        if face.index in visited:
            continue

        stack = [face]
        island_faces = []

        while stack:
            f = stack.pop()
            if f.index in visited:
                continue
            visited.add(f.index)
            island_faces.append(f)

            # Check neighbors ONLY if UVs match on shared vertices
            for edge in f.edges:
                for other_face in edge.link_faces:
                    if other_face.index in visited:
                        continue

                    shared = False
                    for loop in f.loops:
                        luv = loop[uv_layer].uv
                        for loop2 in other_face.loops:
                            if (loop2.vert == loop.vert and
                                (loop2[uv_layer].uv - luv).length < 1e-8):
                                shared = True
                                break
                        if shared:
                            stack.append(other_face)
                            break

        islands.append(island_faces)

    return islands

def get_bounds(island):
    us = [uv.uv.x for uv in island]
    vs = [uv.uv.y for uv in island]
    return min(us), max(us), min(vs), max(vs)

for obj in objects:
    print(f"\nProcessing object: {obj.name}")

    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')

    # Smart UV per object
    bpy.ops.uv.smart_project(
        angle_limit=60,
        island_margin=0.0,
        area_weight=0.0,
        correct_aspect=True,
        scale_to_bounds=False
    )

    bm = bmesh.from_edit_mesh(obj.data)
    uv_layer = bm.loops.layers.uv.active
    
    faces = list(bm.faces)
    visited = set()
    islands = collect_uv_islands(bm, uv_layer)

    # Process each island
    for island in islands:
        uv_loops = [loop[uv_layer] for face in island for loop in face.loops]

        min_u, max_u, min_v, max_v = get_bounds(uv_loops)
        width = max_u - min_u
        height = max_v - min_v

        if width == 0 or height == 0:
            continue

        # Scale to EXACT 32x32
        scale_u = TARGET_SIZE / width
        scale_v = TARGET_SIZE / height

        # Non-uniform scaling allowed (exact tile shape)
        for uv in uv_loops:
            uv.uv.x = (uv.uv.x - min_u) * scale_u
            uv.uv.y = (uv.uv.y - min_v) * scale_v



    bmesh.update_edit_mesh(obj.data)

# Restore mode
bpy.ops.object.mode_set(mode=orig_mode)

print("\nDONE: V7 Auto-Tiled All Selected Objects!")