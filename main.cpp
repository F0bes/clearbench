#include <kernel.h>
#include <stdio.h>
#include <graph.h>
#include <dma.h>
#include <dma_tags.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <gif_tags.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <ee_regs.h>

extern "C"
{
	void _libcglue_timezone_update(void) {}
}

const u32 g_fbptr = 0;
const u32 g_zbptr = 0x46000;
void init_gs()
{
	// Rely on graph to set up privileged registers
	graph_initialize(g_fbptr, 640, 448, GS_PSM_32, 0, 0);

	qword_t *draw_context_pk = static_cast<qword_t *>(aligned_alloc(16, sizeof(qword_t) * 100));
	qword_t *q = draw_context_pk;

	PACK_GIFTAG(q, GIF_SET_TAG(14, 1, GIF_PRE_DISABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1),
				GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_32, 0), GS_REG_FRAME);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(1, 7, 0, 2, 0, 0, 1, 1), GS_REG_TEST);
	q++;
	PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
	q++;
	PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 0, 1, 128), GS_REG_ALPHA);
	q++;
	PACK_GIFTAG(q, GS_SET_ZBUF(g_zbptr >> 11, GS_PSMZ_32, 0), GS_REG_ZBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET);
	q++;
	PACK_GIFTAG(q, GS_SET_DTHE(0), GS_REG_DTHE);
	q++;
	PACK_GIFTAG(q, GS_SET_PRMODECONT(1), GS_REG_PRMODECONT);
	q++;
	PACK_GIFTAG(q, GS_SET_SCISSOR(0, 640 - 1, 0, 448 - 1), GS_REG_SCISSOR);
	q++;
	PACK_GIFTAG(q, GS_SET_CLAMP(0, 0, 0, 0, 0, 0), GS_REG_CLAMP);
	q++;
	PACK_GIFTAG(q, GS_SET_SCANMSK(0), GS_REG_SCANMSK);
	q++;
	PACK_GIFTAG(q, GS_SET_TEXA(0x00, 0, 0x7F), GS_REG_TEXA);
	q++;
	PACK_GIFTAG(q, GS_SET_FBA(0), GS_REG_FBA);
	q++;
	PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, draw_context_pk, q - draw_context_pk, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(draw_context_pk);
}

class ClearMethod
{
public:
	ClearMethod(const char *name, const char *desc, qword_t *prologueStart, qword_t *proglogueEnd, qword_t *clearStart, qword_t *clearEnd)
		: name(name), desc(desc)
	{
		printf("Building test %s with %d qw prologue and %d qw clear\n", name, proglogueEnd - prologueStart, clearEnd - clearStart);
		prologue_size = proglogueEnd - prologueStart;
		prologue = reinterpret_cast<qword_t *>(aligned_alloc(16, sizeof(qword_t) * prologue_size));

		clear_size = clearEnd - clearStart;
		clear = reinterpret_cast<qword_t *>(aligned_alloc(16, sizeof(qword_t) * clear_size));
		memcpy(prologue, prologueStart, sizeof(qword_t) * prologue_size);

		memcpy(clear, clearStart, sizeof(qword_t) * clear_size);
	}

	const char *const get_name() const { return name; }
	const char *const get_desc() const { return desc; }
	// Prologue: Sets up the state for the clear, only need to be run once
	const qword_t *get_prologue() const { return prologue; }
	const size_t get_prologue_size() const { return prologue_size; }
	// Clear: The actual clear, only what needs to be done for the clear, nothing more
	const qword_t *get_clear() const { return clear; }
	const size_t get_clear_size() const { return clear_size; }

private:
	const char *const name;
	const char *const desc;
	qword_t *prologue;
	size_t prologue_size;
	qword_t *clear;
	size_t clear_size;
};

struct Test
{
	u64 dma_time;
	u64 finish_time;
	ClearMethod method;
};

std::vector<Test> g_tests;

void buildClears()
{
	qword_t *prologue = reinterpret_cast<qword_t *>(aligned_alloc(16, sizeof(qword_t) * 20));
	qword_t *prologue_end = nullptr;
	qword_t *clear = reinterpret_cast<qword_t *>(aligned_alloc(16, sizeof(qword_t) * 500));
	qword_t *clear_end = nullptr;

	// Big Sprite
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF(g_zbptr >> 11, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0, 55, 0, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0 << 4, 0 << 4, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(640 << 4, 448 << 4, 0), GS_REG_XYZ2);
		q++;
		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("BigSprite", "The naive approach", prologue, prologue_end, clear, clear_end)});
	}

	// Double Half Clear
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		// 64x32 pages on a 32 bit framebuffer
		// point to the middle of the framebuffer
		// (640x224) / 2048
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11) + 70, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0x37, 0, 0, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0 << 4, 0 << 4, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(640 << 4, 224 << 4, 0x01000037), GS_REG_XYZ2);
		q++;
		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("Double Half Clear", "Set the zbuf to the middle of the frame, clear the top half.", prologue, prologue_end, clear, clear_end)});
	}

	// Page clear
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF(g_zbptr >> 11, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		const u32 page_count = 640 * 448 / 2048;
		PACK_GIFTAG(q, GIF_SET_TAG(page_count + 1, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 2), GIF_REG_AD | (GIF_REG_AD << 4));
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0, 30, 0, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, 0, GS_REG_NOP);
		q++;
		int b = 0;
		for (int i = 0; i < 640; i += 64)
		{
			for (int j = 0; j < 448; j += 32)
			{
				PACK_GIFTAG(q, GS_SET_XYZ(i << 4, j << 4, 0), GS_REG_XYZ2);
				q++;
				PACK_GIFTAG(q, GS_SET_XYZ((i + 64) << 4, (j + 32) << 4, 0), GS_REG_XYZ2);
				q++;
			}
		}
		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("Page Clear", "Clear the framebuffer in 64x32 pages", prologue, prologue_end, clear, clear_end)});
	}

	// Page clear (with dhc opt)
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11) + 70, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		const u32 page_count = 640 * 224 / 2048;
		PACK_GIFTAG(q, GIF_SET_TAG(page_count + 1, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 2), GIF_REG_AD | (GIF_REG_AD << 4));
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0, 00, 0x1E, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, 0, GS_REG_NOP);
		q++;
		int b = 0;
		for (int i = 0; i < 640; i += 64)
		{
			for (int j = 0; j < 224; j += 32)
			{
				PACK_GIFTAG(q, GS_SET_XYZ(i << 4, j << 4, 0), GS_REG_XYZ2);
				q++;
				PACK_GIFTAG(q, GS_SET_XYZ((i + 64) << 4, (j + 32) << 4, 0x011E0000), GS_REG_XYZ2);
				q++;
			}
		}
		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("Page Clear (With DHC)", "The two previous tests together", prologue, prologue_end, clear, clear_end)});
	}
	// Interleaved clear
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(3, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11), GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		const u32 page_count = 640 * 448 / 2048;
		PACK_GIFTAG(q, GIF_SET_TAG(page_count + 1, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 2), GIF_REG_AD | (GIF_REG_AD << 4));
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0x1E, 0x1E, 0x1E, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, 0, GS_REG_NOP);
		q++;
		int b = 0;
		for (int i = 0; i < 640; i += 64)
		{
			for (int j = 0; j < 448; j += 32)
			{
				PACK_GIFTAG(q, GS_SET_XYZ(i << 4, j << 4, 0), GS_REG_XYZ2);
				q++;
				PACK_GIFTAG(q, GS_SET_XYZ((i + 32) << 4, (j + 32) << 4, 0x011E0000), GS_REG_XYZ2);
				q++;
			}
		}
		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("Interleaved Clear", "FBP=ZBP and interleave clear by drawing 32x32 sprites", prologue, prologue_end, clear, clear_end)});
	}
	// VIS clear
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(4, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_zbptr >> 11), GS_PSMZ_32, 1), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		PACK_GIFTAG(q, GS_SET_SCISSOR(0, 64 - 1, 0, 2048 - 1), GS_REG_SCISSOR);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		PACK_GIFTAG(q, GIF_SET_TAG(4, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 3), GIF_REG_AD | (GIF_REG_AD << 4) | (GIF_REG_AD << 8));
		q++;

		PACK_GIFTAG(q, GS_SET_RGBAQ(0x1E, 0x00, 0x00, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((64 << 4), 2048 << 4, 0), GS_REG_XYZ2);
		q++;

		PACK_GIFTAG(q, GS_SET_RGBAQ(0x00, 0x1E, 0x1E, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME((g_fbptr >> 11) + 64, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((64 << 4), 2048 << 4, 0), GS_REG_XYZ2);
		q++;

		PACK_GIFTAG(q, GS_SET_RGBAQ(0x1E, 0x1E, 0x1E, 1, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME((g_fbptr >> 11) + 128, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((64 << 4), 384 << 4, 0), GS_REG_XYZ2);
		q++;

		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("VIS Clear", "FBW = 1 and tall 64x2048 sprites", prologue, prologue_end, clear, clear_end)});
	}
	// VIS clear (Interleaved)
	{
		// Prologue
		qword_t *q = prologue;
		PACK_GIFTAG(q, GIF_SET_TAG(4, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11) + 70, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST);
		q++;
		PACK_GIFTAG(q, GS_SET_SCISSOR(0, 64 - 1, 0, 2048 - 1), GS_REG_SCISSOR);
		q++;
		prologue_end = q;

		// Clear
		q = clear;
		PACK_GIFTAG(q, GIF_SET_TAG(5, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 3), GIF_REG_AD | (GIF_REG_AD << 4) | (GIF_REG_AD << 8));
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0x7F, 0x00, 0x00, 0x7F, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11), GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((32 << 4), 2048 << 4, 0x6000007F), GS_REG_XYZ2);
		q++;

		PACK_GIFTAG(q, GS_SET_RGBAQ(0x00, 0x7F, 0x00, 0x7F, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME((g_fbptr >> 11) + 64, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11) + 64, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((32 << 4), 2048 << 4, 0x60007F00), GS_REG_XYZ2);
		q++;

		PACK_GIFTAG(q, GS_SET_RGBAQ(0x00, 0x00, 0x7F, 0x7F, 1), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_FRAME((g_fbptr >> 11) + 128, 1, GS_PSM_32, 0), GS_REG_FRAME);
		q++;
		PACK_GIFTAG(q, GS_SET_ZBUF((g_fbptr >> 11) + 128, GS_PSMZ_32, 0), GS_REG_ZBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ((32 << 4), 384 << 4, 0x607F0000), GS_REG_XYZ2);
		q++;

		clear_end = q;
		g_tests.push_back(Test{0, 0, ClearMethod("VIS Clear (interleaved)", "The VIS clear with an interleaved approach", prologue, prologue_end, clear, clear_end)});
	}
}

void startTests()
{
	dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
	for (auto &test : g_tests)
	{
		const auto &method = test.method;
		printf("===== Starting test: %s\n", method.get_name());
		printf("	%s\n", method.get_desc());
		dma_channel_send_normal(DMA_CHANNEL_GIF, const_cast<qword_t *>(method.get_prologue()), method.get_prologue_size(), 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		u64 sum = 0;

		printf("   DMA time test starting\n");
		for (int i = 0; i < 500; i++)
		{
			DI();
			graph_wait_vsync();
			u32 start, end;
			*R_EE_D2_MADR = (uiptr)method.get_clear();
			*R_EE_D2_QWC = method.get_clear_size();

			FlushCache(0);

			asm volatile("mfc0 %0, $9" : "=r"(start));
			*R_EE_D2_CHCR = 0x100;
			while (*R_EE_D2_CHCR & 0x100)
				;
			asm volatile("mfc0 %0, $9" : "=r"(end));

			EI();
			sum += end - start;
		}
		printf("   DMA time test completed\n");
		printf("Average time: %d cycles\n", sum / 500);
		test.dma_time = sum / 500;
		printf("  Building DMA chain\n");

		qword_t *chain = static_cast<qword_t *>(aligned_alloc(16, sizeof(qword_t) * 10));
		qword_t *q = chain;
		DMATAG_REF(q, method.get_clear_size(), reinterpret_cast<uiptr>(method.get_clear()), 0, 0, 0);
		q++;
		DMATAG_CNT(q, 2, 0, 0, 0);
		q++;
		PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_FINISH(0), GS_REG_FINISH);
		q++;
		DMATAG_END(q, 0, 0, 0, 0);
		q++;

		printf("   FINISH time test starting\n");

		sum = 0;
		*GS_REG_CSR = 0x2;
		for (int i = 0; i < 500; i++)
		{
			DI();
			graph_wait_vsync();
			u32 start, end;
			*R_EE_D2_TADR = (uiptr)chain;
			*R_EE_D2_QWC = 0;

			asm volatile("mfc0 %0, $9" : "=r"(start));
			*R_EE_D2_CHCR = 0x104;

			asm volatile("la $t0, %1\n"
						 "wait_finish_sig:\n"
						 "lw $t1, 0($t0)\n"
						 "andi $t1, $t1, 0x2\n"
						 "beq $t1, $zero, wait_finish_sig\n"
						 "nop\n"
						 "mfc0 %0, $9\n" : "=r"(end) : "i"(GS_REG_CSR));

			EI();
			sum += end - start;
		}
		free(chain);
		printf("   FINISH time test completed\n");

		printf("Average time: %d cycles\n", sum / 500);
		test.finish_time = sum / 500;
		printf("===== Test done: %s\n", method.get_name());
	}

	printf("===== TEST RESULTS =====\n");
	for (auto &test : g_tests)
	{
		const auto &method = test.method;
		printf("%s (%s)\n", method.get_name(), method.get_desc());
		printf("  DMA time: %d cycles (%dqw)\n", test.dma_time, method.get_clear_size());
		printf("  FINISH time: %d cycles\n", test.finish_time);
	}
}

int main()
{
	init_gs();
	buildClears();
	startTests();
	SleepThread();
	return 0;
}
