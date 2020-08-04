#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int minChar = 32;
int maxChar = 127;

typedef unsigned char byte;
const char* MARK = "**";
const char* SPACE = "  ";

void usage() {
	printf("usage: alf fontfile.ttf fontsize\n");
	printf("fontsize is desired height in px\n");
}

void print_raw_bmp(FT_Bitmap bmp, int left, int top) {
	printf("Glyph size: %d x %d, left %d top %d\n", bmp.width, bmp.rows, left, top);
	return;
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

void print_fs_bmp(FT_Bitmap bmp, int bmpLeft, int bmpTop, int cellWidth, int cellHeight, int cellBaseline) {
	int topRows = cellHeight - bmpTop - cellBaseline;
	int charRows = bmp.rows;
	int bottomRows = cellHeight - topRows - charRows;

	printf("Char %dx%d left %d top %d cell %dx%d -> topr %d, charr %d, botr %d\n", bmp.width,bmp.rows, bmpLeft,bmpTop,cellWidth,cellHeight,topRows,charRows, bottomRows);
	
	for (int y = 0; y < cellHeight; y++) {
		if (y < topRows) {
			printf("T ");
			for (int x = 0; x < cellWidth; x++) printf(SPACE);
		} else if (y < topRows + charRows) {
			int cy = y - topRows;
			printf("%02d", cy);
			for (int x = 0; x < cellWidth; x++) {
				int cx = x - bmpLeft;
				if (x < bmpLeft) {
					printf(SPACE);
				} else if (cx < bmp.width) {
					byte* row = bmp.buffer + (cy * bmp.pitch);
					int o = cx / 8;
					int s = 7 - (cx % 8);
					printf("%s", row[o] & (1 << s) ? MARK : SPACE);
				} else {
					printf(SPACE);
				}
			}
		} else {
			printf("B ");
			for (int x = 0; x < cellWidth; x++) printf(SPACE);
		}
		printf("|\n");
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
	printf("Collecting stats\n");
	
	FT_GlyphSlot slot = face->glyph;
	int minLeft = 1e6;
	int maxRight = -1e6;
	int minTop = 1e6;
	int maxBot = -1e6;

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
		
		int charLeft = slot->bitmap_left;
		int charRight = slot->bitmap_left + slot->bitmap.width;
		int charTop = -slot->bitmap_top;
		int charBot = -slot->bitmap_top + slot->bitmap.rows;

		if (charLeft < minLeft) minLeft = charLeft;
		if (charRight > maxRight) maxRight = charRight;
		if (charTop < minTop) minTop = charTop;
		if (charBot > maxBot) maxBot = charBot;
	}

	printf("Char BB: left %d right %d top %d bot %d\n", minLeft, maxRight, minTop, maxBot);

	int cellWidth = maxRight - minLeft;
	int cellHeight = maxBot - minTop;
	printf("Cell Size: %d x %d\n", cellWidth, cellHeight);
	printf("Rendering into fixed-size cell.\n");
	
	for (int c = minChar; c <= maxChar; c++) {
		int gidx = FT_Get_Char_Index(face, c);
		FT_Load_Glyph(face, gidx, 0);
		FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
		print_fs_bmp(slot->bitmap, slot->bitmap_left, slot->bitmap_top, cellWidth, cellHeight, maxBot);
	}

	return 0;
}
