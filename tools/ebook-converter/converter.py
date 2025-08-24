import ebooklib
from ebooklib import epub
from bs4 import BeautifulSoup
from PIL import Image, ImageDraw, ImageFont
import os

PAGE_SIZE = (480, 800)
MARGIN = 20
LINE_SPACING = 10

font = ImageFont.truetype("fonts/Roboto-Regular.ttf", 28)

def render_text_to_pages(text, font, output_dir="pages"):
    os.makedirs(output_dir, exist_ok=True)

    # Word wrapping
    words = text.split()
    lines, line = [], ""
    for word in words:
        test_line = line + " " + word if line else word
        if font.getlength(test_line) <= PAGE_SIZE[0] - 2*MARGIN:
            line = test_line
        else:
            lines.append(line)
            line = word
    if line:
        lines.append(line)

    # Render pages
    page_num, y = 1, MARGIN
    img = Image.new("L", PAGE_SIZE, "white")  # grayscale
    draw = ImageDraw.Draw(img)

    for line in lines:
        h = font.getbbox(line)[3]
        if y + h > PAGE_SIZE[1] - MARGIN:
            # convert to 1-bit monochrome
            bw = img.convert("1")
            bw.save(f"{output_dir}/page_{page_num:03d}.bmp")   # save as BMP
            page_num += 1
            img = Image.new("L", PAGE_SIZE, "white")
            draw = ImageDraw.Draw(img)
            y = MARGIN
        draw.text((MARGIN, y), line, font=font, fill="black")
        y += h + LINE_SPACING

    # save last page
    bw = img.convert("1")
    bw.save(f"{output_dir}/page_{page_num:03d}.bmp")
    print(f"✅ Rendered {page_num} BMP pages to {output_dir}")


# --- EPUB handling ---
book = epub.read_epub("SachMoi.Net_cuoc-chien-trong-phong-hop-al-ries-laura-ries.epub")

full_text = ""
for item in book.get_items_of_type(ebooklib.ITEM_DOCUMENT):
    soup = BeautifulSoup(item.get_body_content(), "html.parser")
    full_text += soup.get_text() + "\n\n"

render_text_to_pages(full_text, font, "pages_output_bmp")
