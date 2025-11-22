import bpy

def unify_duplicate_materials():

    # Group materials by base name (remove .001, .002…)
    groups = {}

    for mat in bpy.data.materials:
        base = mat.name
        if "." in mat.name:
            parts = mat.name.rsplit(".", 1)
            if parts[1].isdigit():
                base = parts[0]

        if base not in groups:
            groups[base] = []
        groups[base].append(mat)

    # Process groups
    for base, mats in groups.items():
        if len(mats) < 2:
            continue  # no duplicates

        print(f"\n--- Unifying materials: {base} ---")

        master = mats[0]  # keep first material

        for dup in mats[1:]:
            print(f"  Redirecting: {dup.name} → {master.name}")

            # Replace material usage on all objects
            for obj in bpy.data.objects:
                if obj.type != 'MESH':
                    continue

                if not obj.data.materials:
                    continue

                for i, slot in enumerate(obj.data.materials):
                    if slot == dup:
                        obj.data.materials[i] = master

            # Remove duplicate material
            bpy.data.materials.remove(dup)


unify_duplicate_materials()
print("✔ Done! Duplicate materials unified.")
