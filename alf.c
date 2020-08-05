#include <stdio.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int minChar = 32;
int maxChar = 126;

typedef unsigned char byte;
const char* MARK = "**";
const char* SPACE = "  ";

void usage() {
	fprintf(stderr, "usage: alf fontfile.ttf fontsize\n");
	fprintf(stderr, "fontsize is desired height in px\n");
}

void print_raw_glyph_bmp(FT_Bitmap bmp) {
	fprintf(stderr, "Raw glyph size: %d x %d\n", bmp.width, bmp.rows);
	
	for (int y = 0; y < bmp.rows; y++) {
		int bpr = bmp.width / 8 + (bmp.width % 8 ? 1 : 0);
		int w = 0;
		for (int x = 0; x < bpr; x++) {
			unsigned char v = bmp.buffer[y * bmp.pitch + x];
			for (int i = 0; i < 8; i++) {
				printf("%s", (v & 0x80) ? MARK : SPACE);
				v = v << 1;
				if (++w == bmp.width) break;
			}
			printf("\n");
		}
	}
}

void cell_bin_for_row(byte* buff, byte* row, int left, int charWidth, int cellWidth) {
	for (int x = 0; x < cellWidth; x++) {
		byte* b = buff + (x / 8);

		if (x % 8 == 0) *b = 0;
		if (x < left) continue;

		int cx = x - left;
		if (cx >= charWidth) continue;

		if (row[cx / 8] & (1 << (7 - (cx % 8)))) *b = *b | (1 << (7 - (x % 8)));
	}
}

void print_cell_bin_bmp(byte* buff, int cellWidth) {
	for (int x = 0; x < cellWidth; x++) {
		if (buff[x / 8] & (1 << (7 - (x % 8)))) {
			printf("%s", MARK);
		} else {
			printf("%s", SPACE);
		}
	}
}

void print_cell_bin_hex(byte* buff, int cellWidth, char* sep) {
	int byteCount = cellWidth / 8 + ((cellWidth % 8) ? 1 : 0);

	for (int i = 0; i < byteCount; i++) {
		printf("0x%02X%s", buff[i], sep);
	}
}

void print_cell_bmp(FT_Bitmap bmp, int bmpLeft, int bmpTop, int cellWidth, int cellHeight, int cellBaseline) {
	int topRows = cellHeight - bmpTop - cellBaseline;
	int charRows = bmp.rows;
	int bottomRows = cellHeight - topRows - charRows;

	fprintf(stderr, "Char %dx%d left %d top %d in cell %dx%d -> topRows %d, charRows %d, bottomRows %d\n", bmp.width, bmp.rows, bmpLeft, bmpTop, cellWidth, cellHeight, topRows, charRows, bottomRows);

	int binSize = (cellWidth / 8) + (cellWidth % 8 ? 1 : 0);
	byte* buff = malloc(binSize);

	for (int y = 0; y < cellHeight; y++) {
		if (y < topRows) {
			for (int i = 0; i < binSize; i++) buff[i] = 0;
		} else if (y < topRows + charRows) {
			int cy = y - topRows;
			byte* row = bmp.buffer + (cy * bmp.pitch);
			cell_bin_for_row(buff, row, bmpLeft, bmp.width, cellWidth);
		} else {
			for (int i = 0; i < binSize; i++) buff[i] = 0;
		}
		
		printf("|");
		print_cell_bin_bmp(buff, cellWidth);
		printf("| ");
		print_cell_bin_hex(buff, cellWidth, " ");
		printf("\n");
	}
}

void print_cell_fontdata(FT_Bitmap bmp, int bmpLeft, int bmpTop, int cellWidth, int cellHeight, int cellBaseline) {
	int topRows = cellHeight - bmpTop - cellBaseline;
	int charRows = bmp.rows;
	int bottomRows = cellHeight - topRows - charRows;

	int binSize = (cellWidth / 8) + (cellWidth % 8 ? 1 : 0);
	byte* buff = malloc(binSize);

	for (int y = 0; y < cellHeight; y++) {
		if (y < topRows) {
			for (int i = 0; i < binSize; i++) buff[i] = 0;
		} else if (y < topRows + charRows) {
			int cy = y - topRows;
			byte* row = bmp.buffer + (cy * bmp.pitch);
			cell_bin_for_row(buff, row, bmpLeft, bmp.width, cellWidth);
		} else {
			for (int i = 0; i < binSize; i++) buff[i] = 0;
		}

		print_cell_bin_hex(buff, cellWidth, ", ");
		printf(" //");
		print_cell_bin_bmp(buff, cellWidth);
		printf("\n");
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
		fprintf(stderr, "Error initializing FreeType library!\n");
		return 2;
	}
	
	char* fn = argv[1];
	FT_Face face;

	err = FT_New_Face(lib, fn, 0, &face);
	if (err) {
		fprintf(stderr, "Unable to create FreeType face from %s\n", fn);
		return 2;
	}

	fprintf(stderr, "Font Info: %s\n", fn);
	fprintf(stderr, "Number of faces:  %d\n", face->num_faces);
	fprintf(stderr, "Number of glyphs: %d\n", face->num_glyphs);
	fprintf(stderr, "Flags:            0x%04X\n", face->face_flags);
	fprintf(stderr, "Fixed Sizes:      %d\n", face->num_fixed_sizes);
	if (face->num_fixed_sizes) {
		for (int i = 0; i < face->num_fixed_sizes; i++) {
			fprintf(stderr, "    * %dx%d\n", face->available_sizes[i].width, face->available_sizes[i].height);
		}
	}

	int fs = atoi(argv[2]);
	err = FT_Set_Pixel_Sizes(face, 0, fs);
	if (err) {
		fprintf(stderr, "Unable to set size %d for font.\n", fs);
		return 2;
	}
	fprintf(stderr, "\nSet font size to %dpx height\n\n", fs);
	fprintf(stderr, "Collecting stats\n");
	
	FT_GlyphSlot slot = face->glyph;
	int minLeft = 1e6;
	int maxRight = -1e6;
	int minTop = 1e6;
	int maxBot = -1e6;

	for (int c = minChar; c <= maxChar; c++) {
		int gidx = FT_Get_Char_Index(face, c);

		err = FT_Load_Glyph(face, gidx, 0);
		if (err) {
			fprintf(stderr, "Unable to load Glpyh %d into slot\n", gidx);
			return 2;
		}

		err = FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
		if (err) {
			fprintf(stderr, "Unable to render Glyph %d\n", gidx);
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

	fprintf(stderr, "Char BB: left %d right %d top %d bot %d\n", minLeft, maxRight, minTop, maxBot);

	int cellWidth = maxRight - minLeft;
	int cellHeight = maxBot - minTop;
	fprintf(stderr, "Cell Size: %d x %d\n", cellWidth, cellHeight);
	fprintf(stderr, "Rendering into fixed-size cell.\n");
	
	for (int c = minChar; c <= maxChar; c++) {
		int gidx = FT_Get_Char_Index(face, c);
		FT_Load_Glyph(face, gidx, 0);
		FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
		printf("/* Char %d - '%c' */\n", c, c);
		print_cell_fontdata(slot->bitmap, slot->bitmap_left, slot->bitmap_top, cellWidth, cellHeight, maxBot);
	}

	return 0;
}
