#include <stdio.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int minChar = 32;
int maxChar = 126;

typedef unsigned char byte;
const char* MARK = "*";
const char* SPACE = " ";

char transpose = 0;

void usage() {
	fprintf(stderr, "usage: alf fontfile.ttf fontsize [-t]\n");
	fprintf(stderr, "fontsize is desired height in px\n");
	fprintf(stderr, "optional -t transposes font for vertical blitting\n");
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

void char_bin_for_row(byte* buff, byte* row, int charWidth) {
	for (int x = 0; x < charWidth; x++) {
		byte* b = buff + (x / 8);

		if (x % 8 == 0) *b = 0;
		if (x >= charWidth) continue;

		if (row[x / 8] & (1 << (7 - (x % 8)))) *b = *b | (1 << (7 - (x % 8)));
	}
}

void print_char_bin_bmp(byte* buff, int charWidth) {
	for (int x = 0; x < charWidth; x++) {
		if (buff[x / 8] & (1 << (7 - (x % 8)))) {
			printf("%s", MARK);
		} else {
			printf("%s", SPACE);
		}
	}
}

int bytes_for_charWidth(int charWidth) {
	return charWidth / 8 + ((charWidth % 8) ? 1 : 0);
}

void print_char_bin_hex(byte* buff, int charWidth, char* sep) {
	int byteCount = bytes_for_charWidth(charWidth);

	for (int i = 0; i < byteCount; i++) {
		printf("0x%02X%s", buff[i], sep);
	}
}

void print_char_fontdata(FT_Bitmap bmp, int bmpTop, int cellHeight, int cellBaseline) {
	int topRows = cellHeight - bmpTop - cellBaseline;
	int charRows = bmp.rows;
	int bottomRows = cellHeight - topRows - charRows;

	int binSize = bytes_for_charWidth(bmp.width);
	byte* buff = malloc(binSize * cellHeight);

	for (int y = 0; y < cellHeight; y++) {
		if (y < topRows) {
			for (int i = 0; i < binSize; i++) buff[y*binSize+i] = 0;
		} else if (y < topRows + charRows) {
			int cy = y - topRows;
			byte* row = bmp.buffer + (cy * bmp.pitch);
			char_bin_for_row(buff+y*binSize, row, bmp.width);
		} else {
			for (int i = 0; i < binSize; i++) buff[y*binSize+i] = 0;
		}
	}

	if (!transpose) {
		for (int y = 0; y < cellHeight; y++) {
			print_char_bin_hex(buff+y*binSize, bmp.width, ", ");
			printf(" //|");
			print_char_bin_bmp(buff+y*binSize, bmp.width);
			printf("|\n");
		}
	} else {
		int colSize = bytes_for_charWidth(cellHeight);
		byte* outBuff = malloc(colSize);

		for (int x = 0; x < bmp.width; x++) {
			for (int i = 0; i < colSize; i++) outBuff[i] = 0;
			for (int y = 0; y < cellHeight; y++) {
				if (buff[y*binSize + x/8] & (1 << (7-x%8))) {
					outBuff[y/8] += (1 << (y%8));
				} else {
				}
			}
			for (int y = 0; y < colSize; y++) {
				printf("0x%02x, ", outBuff[y]);
			}
			printf("\n");
		}
	}
}

void print_space_fontdata(int charWidth, int cellHeight) {
	int binSize = bytes_for_charWidth(charWidth);
	byte* buff = malloc(binSize);

	for (int y = 0; y < cellHeight; y++) {
		for (int i = 0; i < binSize; i++) buff[i] = 0;

		print_char_bin_hex(buff, charWidth, ", ");
		printf(" //|");
		print_char_bin_bmp(buff, charWidth);
		printf("|\n");
	}
}

int main(int argc, char** argv) {
	if (argc < 3 || argc > 4 || ((argc == 4) && strcmp(argv[3], "-t") != 0)) {
		usage();
		return 1;
	}

	if (argc == 4) transpose = 1;

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
	int minTop = 1e6;
	int maxBot = -1e6;
	int maxWidth = -1e6;
	int *charWidths = malloc(sizeof(int) * (maxChar - minChar + 1));
	int meanWidth = 0;

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
		
		int charTop = -slot->bitmap_top;
		int charBot = -slot->bitmap_top + slot->bitmap.rows;
		int charWidth = slot->bitmap.width;
		charWidths[c - minChar] = charWidth;
		meanWidth += charWidth;

		if (charTop < minTop) minTop = charTop;
		if (charBot > maxBot) maxBot = charBot;
		if (charWidth > maxWidth) maxWidth = charWidth;
	}
	fprintf(stderr, "Char BB: top %d bot %d\n",  minTop, maxBot);

	int cellHeight = maxBot - minTop;
	meanWidth = meanWidth / (maxChar - minChar + 1);
	fprintf(stderr, "Cell Height: %d\nMean Width: %d\n", cellHeight, meanWidth);
	fprintf(stderr, "Rendering into fixed-height cell.\n");
	if (transpose) fprintf(stderr, "Transposing output.\n");

	printf("#include <MonoBitFont.h>\n\n");
	printf("extern \"C\" MonoBitFont *NewFont;\n\n");
	printf("unsigned char fontData[] = {\n");
	int *charOffsets = malloc(sizeof(int) * (maxChar - minChar + 1));
	int currOffset = 0;
	for (int c = minChar; c <= maxChar; c++) {
		int gidx = FT_Get_Char_Index(face, c);
		FT_Load_Glyph(face, gidx, 0);
		FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
		printf("/* Char %d - '%c' */\n", c, c);
		// HACK to fix zero-width space
		if (c == 32 && slot->bitmap.width == 0) {
			print_space_fontdata(meanWidth, cellHeight);
			charWidths[c - minChar] = meanWidth;
			charOffsets[c - minChar] = currOffset;
			currOffset += bytes_for_charWidth(meanWidth) * cellHeight;
		} else {
			print_char_fontdata(slot->bitmap, slot->bitmap_top, cellHeight, maxBot);
			charOffsets[c - minChar] = currOffset;
			currOffset += bytes_for_charWidth(slot->bitmap.width) * cellHeight;
		}
	}
	printf("};\n\n");
	printf("unsigned char offsetData[] = {\n");
	for (int c = minChar; c <= maxChar; c++) {
		printf("0x%04X, ", charOffsets[c - minChar]);
	}
	printf("\n};\n\n");
	printf("unsigned char widthData[] = {\n");
	for (int c = minChar; c <= maxChar; c++) {
		printf("0x%02X, ", charWidths[c - minChar]);
	}
	printf("\n};\n\n");
	printf("MonoBitFont *NewFont = new MonoBitFont(fontData, offsetData, widthData, %d);\n", cellHeight);

	return 0;
}
