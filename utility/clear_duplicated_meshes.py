import bpy

def unify_duplicate_meshes():

    # Group mesh datablocks by base name
    groups = {}

    for mesh in bpy.data.meshes:
        base = mesh.name
        if "." in mesh.name:
            parts = mesh.name.rsplit(".", 1)
            if parts[1].isdigit():
                base = parts[0]

        if base not in groups:
            groups[base] = []
        groups[base].append(mesh)

    # Process each group
    for base, meshes in groups.items():
        if len(meshes) < 2:
            continue

        print(f"\n--- Unifying meshes: {base} ---")

        master = meshes[0]  # Use first mesh as master

        for dup in meshes[1:]:
            print(f"  Replacing mesh data: {dup.name} → {master.name}")

            # Redirect all objects using duplicate mesh
            for obj in bpy.data.objects:
                if obj.type == 'MESH' and obj.data == dup:
                    obj.data = master

            # Remove duplicate mesh datablock
            bpy.data.meshes.remove(dup)


unify_duplicate_meshes()
print("✔ Done! Duplicate mesh datablocks unified.")
