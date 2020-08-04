#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int minChar = 32;
int maxChar = 127;

void usage() {
	printf("usage: alf fontfile.ttf fontsize\n");
	printf("fontsize is desired height in px\n");
}

void print_raw_bmp(FT_Bitmap bmp, int left, int top) {
	printf("Glyph size: %d x %d, left %d top -%d\n", bmp.width, bmp.rows, left, top);
	for (int y = 0; y < bmp.rows; y++) {
		int bpr = bmp.width / 8 + (bmp.width % 8 ? 1 : 0);
		int w = 0;
		for (int x = 0; x < bpr; x++) {
			unsigned char v = bmp.buffer[y * bmp.pitch + x];
			for (int i = 0; i < 8; i++) {
				printf("%c", (v & 0x80) ? '#' : ' ');
				v = v << 1;
				if (++w == bmp.width) break;
			}
			printf("\n");
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 3) {
		usage();
		return 1;
	}

	int err;
	FT_Library lib;

	err = FT_Init_FreeType(&lib);
	if (err) {
		printf("Error initializing FreeType library!\n");
		return 2;
	}
	
	char* fn = argv[1];
	FT_Face face;

	err = FT_New_Face(lib, fn, 0, &face);
	if (err) {
		printf("Unable to create FreeType face from %s\n", fn);
		return 2;
	}

	printf("Font Info: %s\n", fn);
	printf("Number of faces:  %d\n", face->num_faces);
	printf("Number of glyphs: %d\n", face->num_glyphs);
	printf("Flags:            0x%04X\n", face->face_flags);
	printf("Fixed Sizes:      %d\n", face->num_fixed_sizes);
	if (face->num_fixed_sizes) {
		for (int i = 0; i < face->num_fixed_sizes; i++) {
			printf("    * %dx%d\n", face->available_sizes[i].width, face->available_sizes[i].height);
		}
	}

	int fs = atoi(argv[2]);
	err = FT_Set_Pixel_Sizes(face, 0, fs);
	if (err) {
		printf("Unable to set size %d for font.\n", fs);
		return 2;
	}
	printf("\nSet font size to %dpx height\n\n", fs);
	
	FT_GlyphSlot slot = face->glyph;
	int minLeft = 10000;
	int maxWidth = 0;
	int maxTop = 0;;
	int maxBot = 0;
	printf("Collecting stats\n");

	for (int c = minChar; c <= maxChar; c++) {
		int gidx = FT_Get_Char_Index(face, c);
		err = FT_Load_Glyph(face, gidx, 0);
		if (err) {
			printf("Unable to load Glpyh %d into slot\n", gidx);
			return 2;
		}
		err = FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
		if (err) {
			printf("Unable to render Glyph %d\n", gidx);
			return 2;
		}
		if (slot->bitmap_left < minLeft) minLeft = slot->bitmap_left;
		if (slot->bitmap.width + slot->bitmap_left > maxWidth) maxWidth = slot->bitmap.width + slot->bitmap_left;
		if (slot->bitmap_top > maxTop) maxTop = slot->bitmap_top;
		if (slot->bitmap.rows - slot->bitmap_top > maxBot) maxBot = slot->bitmap.rows - slot->bitmap_top; 
	}

	printf("minLeft: %d, maxWidth: %d, maxTop: %d, maxBot: %d\n", minLeft, maxWidth, maxTop, maxBot);
	printf("Cell Size: %d x %d\n", minLeft + maxWidth, maxTop - maxBot);

	return 0;
}
