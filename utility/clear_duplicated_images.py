import bpy
import os

def unify_duplicate_images():

    # Group images by filename without numeric suffix
    groups = {}

    for img in bpy.data.images:
        base = img.name

        # remove Blender numeric suffix: Image.001 → Image
        if "." in img.name:
            parts = img.name.rsplit(".", 1)
            if parts[1].isdigit():
                base = parts[0]

        if base not in groups:
            groups[base] = []
        groups[base].append(img)

    # Process each group
    for base, imgs in groups.items():
        if len(imgs) < 2:
            continue

        print(f"\n--- Unifying images: {base} ---")

        # First image is the master
        master = imgs[0]

        for dup in imgs[1:]:
            print(f"  Redirecting: {dup.name} → {master.name}")

            # Replace usage in nodes
            for mat in bpy.data.materials:
                if not mat.use_nodes:
                    continue

                for node in mat.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image == dup:
                        node.image = master

            # Remove duplicate
            bpy.data.images.remove(dup)


unify_duplicate_images()
print("✔ Done! Duplicate images unified.")
