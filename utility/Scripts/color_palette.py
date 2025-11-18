from PIL import Image, ImageDraw, ImageFont
import colorsys, random, math

CELL = 128
GRID = 32
WIDTH = CELL * GRID
HEIGHT = CELL * GRID

try:
    font = ImageFont.truetype("arial.ttf", 10)
except:
    font = ImageFont.load_default()

###########################################
# AI-LIKE COLOR NAMING BASED ON RGB
###########################################

def generate_name_from_color(rgb):
    r, g, b = rgb
    rf, gf, bf = r/255, g/255, b/255
    h, s, v = colorsys.rgb_to_hsv(rf, gf, bf)

    # convert hue region to category
    hue_deg = h * 360

    # Very bright
    if v > 0.85 and s < 0.2:
        return random.choice([
            "Pure White","Soft Glow","Frost Mist","Snowy Veil",
            "Ivory Light","Moon Glow","Pale Shine","White Frost"
        ])

    # Very dark
    if v < 0.18:
        return random.choice([
            "Deep Shadow","Blackstone","Night Coal","Dark Void",
            "Obsidian Core","Pitch Black","Char Depth","Night Ash"
        ])

    # Red region
    if 0 <= hue_deg < 25 or hue_deg > 340:
        return random.choice([
            "Rust Ember","Burnt Clay","Red Iron","Flame Brick",
            "Warm Ember","Molten Core","Fiery Dust","Cinder Red"
        ])

    # Orange / Brown
    if 25 <= hue_deg < 55:
        return random.choice([
            "Oak Bark","Dry Wood","Amber Soil","Warm Timber",
            "Golden Bark","Autumn Wood","Forest Trunk","Old Oak"
        ])

    # Yellow / Gold
    if 55 <= hue_deg < 90:
        return random.choice([
            "Golden Dust","Shiny Gold","Amber Light","Sungrain",
            "Bright Gold","Soft Honey","Gold Ash","Solar Melt"
        ])

    # Green
    if 90 <= hue_deg < 160:
        return random.choice([
            "Forest Leaf","Moss Patch","Green Moss","Fresh Grass",
            "Jungle Tint","Deep Grove","Verdant Root","Wild Herb"
        ])

    # Cyan / Teal
    if 160 <= hue_deg < 200:
        return random.choice([
            "Cold Glacier","Teal Mist","Frozen Lake","Icy Water",
            "Glacial Wash","Mint Stone","Aqua Drift","Winter Stream"
        ])

    # Blue
    if 200 <= hue_deg < 255:
        return random.choice([
            "Deep Ocean","Cold Steel Blue","Royal Sapphire","Horizon Sea",
            "Blue Current","Night Sea","Ocean Depth","Azure Crest"
        ])

    # Purple
    if 255 <= hue_deg < 300:
        return random.choice([
            "Mystic Violet","Deep Amethyst","Shadow Purple","Moon Violet",
            "Dark Magenta","Twilight Mist","Royal Indigo","Spirit Bloom"
        ])

    # Pink
    if 300 <= hue_deg < 340:
        return random.choice([
            "Cherry Bloom","Soft Petal","Rose Mist","Floral Pink",
            "Warm Blossom","Orchid Tint","Petal Blush","Faded Rose"
        ])

    # Fallback neutral
    return random.choice([
        "Stone Grey","Soft Dust","Matte Steel","Cold Pebble",
        "Light Ash","Soft Slate","Warm Fog","Neutral Clay"
    ])


###########################################
# GENERATE V6 VARIANTS (same as before)
###########################################

def generate_v6_variants(base_rgb, count):
    r, g, b = [x/255 for x in base_rgb]
    h, s, v = colorsys.rgb_to_hsv(r, g, b)

    variants = []
    hue_amplitude = 0.015

    for i in range(count):
        t = i / (count - 1)
        h2 = h + math.sin(t * math.pi * 2) * hue_amplitude
        h2 += random.uniform(-0.003, 0.003)
        s2 = s + (math.sin(t * math.pi) * 0.12) + (math.sin(t * 4 * math.pi) * 0.03)
        s2 += random.uniform(-0.02, 0.02)
        s2 = min(max(s2, 0), 1)

        val_curve = (t*t * (3 - 2*t))
        v2 = 0.55*(1-val_curve) + 1.00*val_curve
        v2 += math.sin(t * math.pi * 3) * 0.04
        v2 += random.uniform(-0.02, 0.02)
        v2 = min(max(v2, 0), 1)

        rr, gg, bb = colorsys.hsv_to_rgb(h2 % 1, s2, v2)
        variants.append((
            min(max(int(rr*255 + random.randint(-1,1)),0),255),
            min(max(int(gg*255 + random.randint(-1,1)),0),255),
            min(max(int(bb*255 + random.randint(-1,1)),0),255)
        ))

    return variants


###########################################
# Category list
###########################################

categories = list({
    "Wood"        : (139, 69, 19),
    "DarkWood"    : (70, 40, 15),
    "Iron"        : (155, 155, 165),
    "Steel"       : (110, 110, 120),
    "Copper"      : (184, 115, 51),
    "Bronze"      : (150, 90, 40),
    "Gold"        : (255, 215, 0),
    "Silver"      : (192, 192, 192),
    "Soil"        : (120, 80, 40),
    "Mud"         : (95, 70, 40),
    "Sand"        : (210, 190, 130),
    "Rock"        : (100, 100, 105),
    "Stone"       : (130, 130, 135),
    "Granite"     : (145, 145, 150),
    "Marble"      : (220, 220, 230),
    "Leather"     : (145, 90, 50),
    "DarkLeather" : (80, 50, 30),
    "Leaf"        : (34, 139, 34),
    "Grass"       : (70, 160, 60),
    "Moss"        : (90, 130, 60),
    "Water"       : (30, 80, 180),
    "DeepWater"   : (10, 40, 110),
    "Ice"         : (180, 220, 250),
    "Snow"        : (240, 240, 255),
    "Clay"        : (165, 100, 60),
    "Brick"       : (160, 50, 40),
    "Charcoal"    : (40, 40, 40),
    "Ash"         : (170, 170, 160),
    "Bone"        : (230, 225, 200),
    "Fur"         : (180, 150, 120),
    "Fabric"      : (180, 80, 80),
    "Obsidian"    : (20, 20, 40),
}.items())

# extend to 32 rows by repeating bases
while len(categories) < GRID:
    categories.append(categories[-1])


###########################################
# DRAW IMAGE
###########################################

img = Image.new("RGB", (WIDTH, HEIGHT))
draw = ImageDraw.Draw(img)

for y in range(GRID):
    base_name, base_color = categories[y]
    row_colors = [base_color] + generate_v6_variants(base_color, GRID-1)

    for x in range(GRID):
        color = row_colors[x]
        x0 = x * CELL
        y0 = y * CELL
        x1 = x0 + CELL
        y1 = y0 + CELL

        # fill cell
        draw.rectangle([x0, y0, x1, y1], fill=color)

        # label
        name = generate_name_from_color(color)

        # contrast auto
        bright = color[0]*0.299 + color[1]*0.587 + color[2]*0.114
        txt = (255,255,255) if bright < 128 else (0,0,0)

        draw.text((x0+3, y0+3), name, fill=txt, font=font)

img.save("ai_named_palette_128px.png")
print("Generated ai_named_palette_128px.png")
