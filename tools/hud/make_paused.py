"""Renders data/menus/paused.psd, the story mode's pause overlay, in the look of the countdown's "GO".

The countdown's digits are pre-rendered bitmaps (data/menus/countdown.psd), their font is the game's
"Dynablaster Revolution". This script draws "PAUSED" in that font with the same orange gradient,
yellow rim, black outline and downward extrusion, measured from the "go" layer.

usage: python tools/hud/make_paused.py <Dynablaster Revolution.ttf> [out.psd]
the font lives in the history repo: version1/dstar-projects/FINAL/font/Dynablaster Revolution (1.00).ttf
"""

import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont
from psd_tools import PSDImage
from psd_tools.constants import Compression
from psd_tools.api.layers import PixelLayer

TEXT = "PAUSED"
SCREEN = (1920, 1080)
CENTER = (960, 540)  # same place as the countdown

GLYPH_HEIGHT = 78  # cap height of "GO" before emboldening
BOLD = 2  # "GO" is a bolder cut than the font
TRACKING = 14  # extra space between the letters, keeps their outlines apart like in "GO"
RIM = 2  # yellow edge inside the glyph
OUTLINE = 6  # black around the glyph
EXTRUSION = 21  # black below the glyph

# fill colour over the glyph's height, measured from "GO"
FILL = [
   (0.00, (212, 92, 4)),
   (0.16, (193, 82, 0)),
   (0.30, (210, 90, 1)),
   (0.50, (221, 129, 59)),
   (0.52, (229, 163, 137)),
   (0.70, (221, 131, 83)),
   (0.89, (208, 88, 5)),
   (1.00, (198, 84, 0)),
]
RIM_COLORS = [(0.0, (255, 245, 125)), (0.5, (255, 184, 10)), (1.0, (255, 142, 0))]


def gradient(stops, t):
   for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
      if t <= t1:
         f = 0.0 if t1 == t0 else (t - t0) / (t1 - t0)
         return tuple(round(a + (b - a) * f) for a, b in zip(c0, c1))
   return stops[-1][1]


def vertical(stops, size, top, height):
   column = Image.new("RGB", (1, size[1]))
   for y in range(size[1]):
      column.putpixel((0, y), gradient(stops, min(max((y - top) / height, 0.0), 1.0)))
   return column.resize(size)


def glyph_mask(font_path):
   # find the font size whose capitals are GLYPH_HEIGHT tall
   size = 100
   for _ in range(3):
      box = ImageFont.truetype(font_path, size).getbbox(TEXT)
      size = round(size * GLYPH_HEIGHT / (box[3] - box[1]))
   font = ImageFont.truetype(font_path, size)
   box = font.getbbox(TEXT)
   pad = OUTLINE + EXTRUSION + 4
   width = box[2] - box[0] + TRACKING * (len(TEXT) - 1)
   mask = Image.new("L", (width + 2 * pad, box[3] - box[1] + 2 * pad), 0)
   draw = ImageDraw.Draw(mask)
   x = pad - box[0]
   for letter in TEXT:
      draw.text((x, pad - box[1]), letter, font=font, fill=255)
      x += font.getlength(letter) + TRACKING
   return mask.filter(ImageFilter.MaxFilter(2 * BOLD + 1)), pad


def render(font_path):
   glyph, pad = glyph_mask(font_path)
   size = glyph.size
   height = size[1] - 2 * pad

   outline = glyph.filter(ImageFilter.MaxFilter(2 * OUTLINE + 1))
   black = outline.copy()
   for dy in range(1, EXTRUSION + 1):
      black = ImageChops.lighter(black, ImageChops.offset(outline, 0, dy))

   inner = glyph.filter(ImageFilter.MinFilter(2 * RIM + 1))
   image = Image.new("RGBA", size, (0, 0, 0, 0))
   image.paste((0, 0, 0, 255), mask=black)
   image.paste(vertical(RIM_COLORS, size, pad, height), mask=glyph)
   image.paste(vertical(FILL, size, pad, height), mask=inner)
   return image.crop(image.getbbox()), pad


def main():
   font_path = sys.argv[1]
   out = sys.argv[2] if len(sys.argv) > 2 else "client/data/menus/paused.psd"
   image, _ = render(font_path)

   # an RGBA document keeps the alpha as the layer's transparency channel, RGB would add a user mask
   # the game's loader doesn't read
   psd = PSDImage.new("RGBA", SCREEN)
   left = CENTER[0] - image.width // 2
   top = CENTER[1] - (image.height - EXTRUSION) // 2
   PixelLayer.frompil(image, psd, "paused", top, left)

   # the merged image is empty, RLE keeps it from taking megabytes
   merged = psd._record.image_data
   data = merged.get_data(psd._record.header)
   merged.compression = Compression.RLE
   merged.set_data(data, psd._record.header)
   psd.save(out)
   print(f"{out}: paused {image.width}x{image.height} at {left},{top}")


if __name__ == "__main__":
   main()
