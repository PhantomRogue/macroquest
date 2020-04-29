/*
 * MacroQuest2: The extension platform for EverQuest
 * Copyright (C) 2002-2019 MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "pch.h"
#include "MQ2DeveloperTools.h"

#include "imgui/ImGuiUtils.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace mq {

static void DeveloperTools_Initialize();
static void DeveloperTools_Shutdown();
static void DeveloperTools_SetGameState(DWORD gameState);

static MQModule s_developerToolsModule = {
	"DeveloperTools",              // Name
	true,                          // CanUnload
	DeveloperTools_Initialize,
	DeveloperTools_Shutdown,
	nullptr,
	DeveloperTools_SetGameState,
};
DECLARE_MODULE_INITIALIZER(s_developerToolsModule);

//----------------------------------------------------------------------------

#pragma region Common Tools

static void ColumnValue(const char* fmt, va_list args)
{
	if (fmt[0] == '(' && (strcmp(fmt, "(empty)") == 0 || strcmp(fmt, "(null)") == 0))
	{
		ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "%s", fmt);
	}
	else if (strcmp(fmt, "%s") == 0)
	{
		const char* str = va_arg(args, const char*);

		if (strcmp(str, "(empty)") == 0 || strcmp(str, "(null)") == 0)
			ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), str);
		else
			ImGui::Text("%s", str);
	}
	else
	{
		ImGui::TextV(fmt, args);
	}
}

static void ColumnText(const char* Label, const char* fmt, ...)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	va_list args;
	va_start(args, fmt);
	ColumnValue(fmt, args);
	va_end(args);

	ImGui::TableNextRow();
}

static void ColumnTextType(const char* Label, const char* Type, const char* fmt, ...)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	va_list args;
	va_start(args, fmt);
	ColumnValue(fmt, args);
	va_end(args);
	ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), Type);
	ImGui::TableNextRow();
}

static bool ColumnCheckBox(const char* Label, bool* value)
{
	bool result = false;
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();
	ImGui::PushID(Label); result = ImGui::Checkbox("", value); ImGui::PopID();
	ImGui::TableNextRow();
	return result;
}

static bool ColumnCheckBoxFlags(const char* Label, unsigned int* flags, unsigned int flags_value)
{
	bool result = false;
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();
	ImGui::PushID(Label); result = ImGui::CheckboxFlags("", flags, flags_value); ImGui::PopID();
	ImGui::TableNextRow();
	return result;
}

static bool ColumnTreeNode(const char* Label, const char* fmt, ...)
{
	bool result = ImGui::TreeNode(Label); ImGui::TableNextCell();

	va_list args;
	va_start(args, fmt);
	ColumnValue(fmt, args);
	va_end(args);

	ImGui::TableNextRow();
	return result;
}

static bool ColumnTreeNodeType(const char* Label, const char* Type, const char* fmt, ...)
{
	bool result = ImGui::TreeNode(Label); ImGui::TableNextCell();

	va_list args;
	va_start(args, fmt);
	ColumnValue(fmt, args);
	va_end(args);
	ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), Type);
	ImGui::TableNextRow();
	return result;
}

static bool ColumnTreeNodeType2(const void* Id, const char* Label, const char* Type, const char* fmt, ...)
{
	bool result = ImGui::TreeNode(Id, Label); ImGui::TableNextCell();

	va_list args;
	va_start(args, fmt);
	ColumnValue(fmt, args);
	va_end(args);
	ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), Type);
	ImGui::TableNextRow();
	return result;
}

static bool InputCXRect(const char* label, CXRect& rect)
{
	return ImGui::InputInt4(label, (int*)&rect);
}

inline bool InputCXSize(const char* label, CXSize& size)
{
	return ImGui::InputInt2(label, (int*)&size);
}

inline bool InputCXPoint(const char* label, CXPoint& point)
{
	return ImGui::InputInt2(label, (int*)&point);
}

inline bool InputColorRef(const char* label, COLORREF& color)
{
	ImColor colors{ color };

	if (ImGui::ColorEdit4(label, (float*)&colors))
	{
		color = colors;
		return true;
	}

	return false;
}

inline void ColumnCXStr(const char* Label, const CXStr& str)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	if (str.empty())
		ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "(empty)");
	else
		ImGui::Text("%s", str.c_str());
	ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "CXStr");
	ImGui::TableNextRow();
}

inline void ColumnCXSize(const char* Label, const CXSize& size)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	ImGui::Text("{ w=%d, h=%d }", size.cx, size.cy); ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "CXSize");
	ImGui::TableNextRow();
}

inline void ColumnCXRect(const char* Label, const CXRect& rect)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	ImGui::Text("{ x=%d, y=%d, w=%d, h=%d }", rect.left, rect.top, rect.GetWidth(), rect.GetHeight()); ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "CXRect");
	ImGui::TableNextRow();
}

inline void ColumnCXPoint(const char* Label, const CXPoint& point)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	ImGui::Text("{ x=%d, y=%d }", point.x, point.y); ImGui::TableNextCell();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "CXPoint");
	ImGui::TableNextRow();
}

inline void ColumnColor(const char* Label, const COLORREF& color)
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	//ImGui::ColorButton(id, ImGui::ColorConvertU32ToFloat4(color),
	//ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags = 0, ImVec2 size = ImVec2(0, 0));

	// colors are in BGRA form

	ImGui::PushID(Label);
	ImColor colors(
		(int)((color >> 16) & 0xff),
		(int)((color >> 8) & 0xff),
		(int)(color & 0xff),
		(int)((color >> 24) & 0xff)
	);

	ImGui::ColorEdit4("", (float*)&colors, ImGuiColorEditFlags_NoInputs); ImGui::TableNextCell();
	ImGui::PopID();

	ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, .5f), "Color");
	ImGui::TableNextRow();
}

//----------------------------------------------------------------------------

inline const char* UIDirectoryToString(enDir dir)
{
	switch (dir)
	{
	case cUIDirectory: return "UI Directory";
	case cUIDirectoryAtlas: return "Altas Directory";
	case cUIDirectoryTexture: return "Texture Directory";
	case cUIDirectoryMaps: return "Maps Directory";
	default: return "Unknown Directory";
	}
}

bool RenderUITextureInfoTexture(const CUITextureInfo& textureInfo, const CXRect& rect = CXRect(0, 0, -1, -1))
{
	if (textureInfo.TextureId == -1)
		return false;

	// Try to extract texture info from the texture manager, if it exists already.
	// This won't work at login until we map the texture loader to the login instance.

	if (!pEQSuiteTextureLoader)
		return false;

	BMI* bitmap = pEQSuiteTextureLoader->GetTexture(textureInfo);
	if (!bitmap)
		return false;

	CEQGBitmap* pEQGBitmap = bitmap->pBmp;
	if (!pEQGBitmap)
		return false;

	if (pEQGBitmap->pD3DTexture == nullptr)
		return false;

	ImVec2 minUV = ImVec2(0, 0);
	ImVec2 maxUV = ImVec2(1, 1);
	ImVec2 textureSize = ImVec2((float)textureInfo.TextureSize.cx, (float)textureInfo.TextureSize.cy);

	if (!rect.IsAbnormal())
	{
		minUV.x = rect.left / textureSize.x;
		minUV.y = rect.top / textureSize.y;

		maxUV.x = rect.right / textureSize.x;
		maxUV.y = rect.bottom / textureSize.y;

		textureSize.x = (float)rect.GetWidth();
		textureSize.y = (float)rect.GetHeight();
	}

	ImGui::Image((ImTextureID)pEQGBitmap->pD3DTexture, textureSize, minUV, maxUV, ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f));
	return true;
}

bool ColumnTextureInfoPreview(const char* Label, const CUITextureInfo& textureInfo, const CXRect& rect = CXRect(0, 0, -1, -1))
{
	ImGui::TreeAdvanceToLabelPos(); ImGui::Text(Label); ImGui::TableNextCell();

	bool result = RenderUITextureInfoTexture(textureInfo, rect);
	ImGui::TableNextRow();
	return result;
}

void DisplayUITextureInfo(const char* Label, const CUITextureInfo& textureInfo)
{
	bool show = ColumnTreeNodeType2((const void*)&textureInfo, Label, "CUITextureInfo", "%s", textureInfo.Name.c_str());
	if (show)
	{
		ColumnCXStr("Name", textureInfo.Name);
		ColumnText("Valid", "%s", textureInfo.bValid ? "true" : "false");
		ColumnText("Directory", "%s", UIDirectoryToString(textureInfo.Directory));
		ColumnCXSize("Texture Size", textureInfo.TextureSize);
		ColumnText("Texture Id", "%d", textureInfo.TextureId);
		ColumnTextureInfoPreview("Texture", textureInfo);

		ImGui::TreePop();
	}
}

void DisplayUITexturePiece(const char* Label, const CUITexturePiece& texturePiece)
{
	bool show = ColumnTreeNodeType2((const void*)&texturePiece, Label, "CUITexturePiece", "%s", texturePiece.GetTextureInfo().Name.c_str());
	if (show)
	{
		ColumnCXStr("Name", texturePiece.GetTextureInfo().Name);
		DisplayUITextureInfo("TextureInfo", texturePiece.GetTextureInfo());
		ColumnCXRect("Rect", texturePiece.GetRect());
		ColumnCXSize("Size", texturePiece.GetSize());
		ColumnTextureInfoPreview("Preview", texturePiece.GetTextureInfo(), texturePiece.GetRect());

		ImGui::TreePop();
	}
}

void DisplaySTextureAnimationFrame(int frameId, const STextureAnimationFrame& frame, bool showNumber)
{
	char labelText[20];
	if (showNumber)
		sprintf_s(labelText, "Frame %d", frameId + 1);
	else
		strcpy_s(labelText, "Frame");

	bool showFrame = ColumnTreeNodeType2((const void*)&frame, labelText, "STextureAnimationFrame", "");
	if (showFrame)
	{
		DisplayUITexturePiece("TexturePiece", frame.Piece);
		ColumnText("Ticks", "%d", frame.Ticks);
		ColumnCXPoint("Hotspot", frame.Hotspot);

		ImGui::TreePop();
	}
}

void DisplayTextureAnimation(const char* Label, const CTextureAnimation* textureAnim, bool doEmpty = true)
{
	if (!textureAnim)
	{
		if (doEmpty)
			ColumnTextType(Label, "CTextureAnimation", "(null)");
		return;
	}

	bool show = ColumnTreeNodeType(Label, "CTextureAnimation", "%s", textureAnim->Name.c_str());
	if (show)
	{
		ColumnCXStr("Name", textureAnim->Name);

		if (textureAnim->Frames.GetLength() > 1)
		{
			bool showFrames = ColumnTreeNodeType("Frames", "STextureAnimationFrame[]", "%d", textureAnim->Frames.GetLength());
			if (showFrames)
			{
				for (int i = 0; i < textureAnim->Frames.GetLength(); ++i)
				{
					const STextureAnimationFrame& frame = textureAnim->Frames[i];

					DisplaySTextureAnimationFrame(i, frame, true);
				}

				ImGui::TreePop();
			}
		}
		else
		{
			const STextureAnimationFrame& frame = textureAnim->Frames[0];

			DisplaySTextureAnimationFrame(0, frame, false);
		}

		ColumnCXSize("Size", textureAnim->Size);
		ColumnText("Paused", textureAnim->bPaused ? "true" : "false");
		ColumnText("Vertical", textureAnim->bVertical ? "true" : "false");
		ColumnText("Grid", textureAnim->bGrid ? "true" : "false");

		if (textureAnim->bGrid)
		{
			ColumnCXSize("Cell size", CXSize(textureAnim->CellWidth, textureAnim->CellHeight));
			ColumnCXRect("Cell rect", textureAnim->CellRect);
			ColumnText("Current cell", "%d", textureAnim->CurCell);
		}

		if (textureAnim->ZeroFrame != 0)
		{
			ColumnText("Zero frame", "%d", textureAnim->ZeroFrame);
		}

		ColumnText("Cycle animation", textureAnim->bCycle ? "true" : "false");
		ColumnText("Total ticks", "%d", textureAnim->TotalTicks);
		ColumnText("Start ticks", "%d", textureAnim->StartTicks);

		ImGui::TreePop();
	}
}

void DisplayTAFrameDraw(const char* Label, const CTAFrameDraw& frameDraw)
{
	bool show = ColumnTreeNodeType2((const void*)&frameDraw, Label, "CTAFrameDraw", "%s", frameDraw.GetName().c_str());
	if (show)
	{
		ColumnCXStr("Name", frameDraw.GetName());
		ColumnCXSize("Frame size", frameDraw.GetFrameSize());

		for (int i = 0; i < CTAFrameDraw::FrameDraw_Max; ++i)
		{
			CTextureAnimation* anim = frameDraw.GetAnimation(static_cast<CTAFrameDraw::EFrameDrawPiece>(i));
			if (!anim)
				continue;

			DisplayTextureAnimation(
				CTAFrameDraw::FrameDrawPieceToString(static_cast<CTAFrameDraw::EFrameDrawPiece>(i)),
				anim);
		}

		ImGui::TreePop();
	}
}

void DisplayDrawTemplate(const char* label, const CButtonDrawTemplate& drawTemplate)
{

	bool show = ColumnTreeNodeType2((const void*)&drawTemplate, label, "CButtonDrawTemplate", "%s", drawTemplate.strName.c_str());
	if (show)
	{
		ColumnCXStr("Name", drawTemplate.strName);
		DisplayTextureAnimation("Normal", drawTemplate.ptaNormal, false);
		DisplayTextureAnimation("Pressed", drawTemplate.ptaPressed, false);
		DisplayTextureAnimation("Hover", drawTemplate.ptaFlyby, false);
		DisplayTextureAnimation("Disabled", drawTemplate.ptaDisabled, false);
		DisplayTextureAnimation("Pressed hover", drawTemplate.ptaPressedFlyby, false);
		DisplayTextureAnimation("Pressed disabled", drawTemplate.ptaPressedDisabled, false);
		DisplayTextureAnimation("Normal decal", drawTemplate.ptaNormalDecal, false);
		DisplayTextureAnimation("Pressed decal", drawTemplate.ptaPressedDecal, false);
		DisplayTextureAnimation("Hover decal", drawTemplate.ptaFlybyDecal, false);
		DisplayTextureAnimation("Disabled decal", drawTemplate.ptaDisabledDecal, false);
		DisplayTextureAnimation("Pressed hover decal", drawTemplate.ptaPressedFlybyDecal, false);
		DisplayTextureAnimation("Pressed disabled decal", drawTemplate.ptaPressedDisabledDecal, false);

		ImGui::TreePop();
	}
}

void DisplayDrawTemplate(const char* label, const CSpellGemDrawTemplate& drawTemplate)
{
	if (ColumnTreeNodeType2((const void*)&drawTemplate, label, "CSpellGemDrawTemplate", "%s", drawTemplate.strName.c_str()))
	{
		ColumnCXStr("Name", drawTemplate.strName);

		DisplayTextureAnimation("Background", drawTemplate.ptaBackground);
		DisplayTextureAnimation("Holder", drawTemplate.ptaHolder);
		DisplayTextureAnimation("Highlight", drawTemplate.ptaHighlight);

		ImGui::TreePop();
	}
}

void DisplayDrawTemplate(const char* label, const CScrollbarTemplate& drawTemplate)
{
	bool show = ColumnTreeNodeType2((const void*)&drawTemplate, label, "CScrollbarTemplate", "%s", drawTemplate.strName.c_str());
	if (show)
	{
		ColumnCXStr("Name", drawTemplate.strName);
		DisplayDrawTemplate("Up Button", drawTemplate.bdtUp);
		DisplayDrawTemplate("Down Button", drawTemplate.bdtDown);
		DisplayTAFrameDraw("Thumb", drawTemplate.frameThumb);
		DisplayUITextureInfo("Middle", drawTemplate.tiMiddle);
		ColumnColor("Middle tint", drawTemplate.colorMiddleTint);

		ImGui::TreePop();
	}
}

void DisplayDrawTemplate(const char* label, const CXWndDrawTemplate* drawTemplate)
{
	if (!drawTemplate)
	{
		ColumnTextType(label, "CXWndDrawTemplate", "(none)");
		return;
	}

	bool show = ColumnTreeNodeType2((const void*)drawTemplate, label, "CXWndDrawTemplate", "%s", drawTemplate->strName.c_str());
	if (show)
	{
		ColumnCXStr("Name", drawTemplate->strName);
		DisplayUITextureInfo("Background texture", drawTemplate->tiBackground);
		ColumnText("Background type",
			XWndBackgroundDrawTypeToString(static_cast<XWndBackgroundDrawType>(drawTemplate->nBackgroundDrawType)));
		DisplayDrawTemplate("Vertical scroll bar", drawTemplate->sbtVScroll);
		DisplayDrawTemplate("Horizontal scroll bar", drawTemplate->sbtHScroll);
		DisplayDrawTemplate("Close box", drawTemplate->bdtCloseBox);
		DisplayDrawTemplate("Help box", drawTemplate->bdtQMarkBox);
		DisplayDrawTemplate("Minimize box", drawTemplate->bdtMinimizeBox);
		DisplayDrawTemplate("Maximize box", drawTemplate->bdtMaximizeBox);
		DisplayDrawTemplate("Tile box", drawTemplate->bdtTileBox);
		DisplayTAFrameDraw("Border", drawTemplate->frameBorder);
		DisplayTAFrameDraw("Title bar", drawTemplate->frameTitlebar);

		ImGui::TreePop();
	}
}

void DisplayScreenTemplate(const char* label, const CScreenTemplate* pTemplate)
{
	if (!pTemplate)
	{
		ColumnTextType(label, "CScreenTemplate", "(none)");
		return;
	}

	if (ColumnTreeNodeType2((const void*)pTemplate, label, "CScreenTemplate", "%s", pTemplate->strName.c_str()))
	{
		ColumnCXStr("Name", pTemplate->strName);

		// TODO: fill me in.

		ImGui::TreePop();
	}
}

#pragma endregion

#pragma region Property Viewer

class ImGuiWindowPropertyViewer
{
	CXWnd* m_window = nullptr;

	inline static ImU32 s_propertyColors[] = {
		(ImU32)ImColor(4, 32, 39, 120),
		(ImU32)ImColor(39, 32, 4, 120),
		(ImU32)ImColor(70, 23, 10, 80),
		(ImU32)ImColor(42, 20, 68, 80),
		(ImU32)ImColor(66, 68, 20, 80),
		(ImU32)ImColor(68, 20, 67, 80),
	};
	int m_currentColor = 0;

public:
	ImGuiWindowPropertyViewer() = default;

	void SetWindow(CXWnd* pWindow)
	{
		m_window = pWindow;
	}

	CXWnd* GetWindow() const { return m_window; }

	void Draw()
	{
		m_currentColor = 0;

		if (m_window)
		{
			ImGui::Text("Selected Window: %s", m_window->GetXMLName().c_str());
		}
		else
		{
			ImGui::Text("Select a window to see details");
		}

		ImGui::Separator();

		DisplayPropertiesPanel();
	}

	void DisplayPropertiesPanel()
	{
		if (!m_window)
		{
			return;
		}

		ImGuiTableFlags tableFlags =
			ImGuiTableFlags_ScrollFreezeTopRow
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_BordersV
			| ImGuiTableFlags_BordersHOuter
			| ImGuiTableFlags_BordersHInner
			| ImGuiTableFlags_Resizable
			| ImGuiTableFlags_RowBg
			;


		// Set up a table with three columns: Name, Value, Type
		if (!ImGui::BeginTable("##WindowDetailsTable", 3, tableFlags))
		{
			ImGui::EndTable();
			return;
		}

		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 150);
		ImGui::TableAutoHeaders();
		ImGui::TableNextRow();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);

		switch (m_window->GetType())
		{
		case UI_Screen:
			DisplayDetailsSection(static_cast<CSidlScreenWnd*>(m_window));
			break;

		case UI_Button:
			DisplayDetailsSection(static_cast<CButtonWnd*>(m_window));
			break;

		case UI_SpellGem:
			DisplayDetailsSection(static_cast<CSpellGemWnd*>(m_window));
			break;

		default:
			DisplayDetailsSection(m_window);
			break;
		}

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}

	bool BeginColorSection(const char* properties, bool open)
	{
		int color = m_currentColor++;
		if (m_currentColor >= (int)lengthof(s_propertyColors))
			m_currentColor = 0;

		ImColor bgColor(0, 0, 0, 0);
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, (ImU32)bgColor);
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, (ImU32)bgColor);

		bool show = ImGui::CollapsingHeader(properties, open ? ImGuiTreeNodeFlags_DefaultOpen : 0);
		ImGui::TableNextRow();

		ImGui::PopStyleColor(2);

		if (show)
		{
			ImColor rowColor = s_propertyColors[color];
			ImGui::PushStyleColor(ImGuiCol_TableRowBg, (ImU32)rowColor);
			ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, (ImU32)rowColor);
		}

		return show;
	}

	void EndColorSection()
	{
		ImGui::PopStyleColor(2);
	}

	void DisplayDetailsSection(CXMLData* pXMLData, bool open = true)
	{
		if (!pXMLData) return;

		if (BeginColorSection("XML Properties", open))
		{
			// Type
			ColumnText("Type", "%s (%d)", pXMLData->TypeName.c_str(), pXMLData->Type);

			// Name
			ColumnText("Name", "%s", pXMLData->Name.c_str());

			// ScreenID
			ColumnText("Screen ID", "%s", pXMLData->ScreenID.c_str());

			EndColorSection();
		}
	}

	void DisplayDetailsSection(CXWnd* pWnd, bool open = true)
	{
		DisplayDetailsSection(pWnd->GetXMLData(), open);

		// Add CXWnd specific details here
		if (BeginColorSection("CXWnd Properties", open))
		{
			ColumnCXStr("Text", pWnd->WindowText);
			ColumnCXStr("Tooltip", pWnd->Tooltip);

			ColumnCXRect("Location", pWnd->Location);
			ColumnCXRect("Client rect", pWnd->ClientRect);
			ColumnText("Z layer", "%d", pWnd->ZLayer);

			// Style
			if (ColumnTreeNode("Style", "0x%08x", pWnd->WindowStyle))
			{
				ColumnCheckBoxFlags("Vertical Scroll", &pWnd->WindowStyle, CWS_VSCROLL);
				ColumnCheckBoxFlags("Horizontal Scroll", &pWnd->WindowStyle, CWS_HSCROLL);
				ColumnCheckBoxFlags("Title Bar", &pWnd->WindowStyle, CWS_TITLE);
				ColumnCheckBoxFlags("Close Box", &pWnd->WindowStyle, CWS_CLOSE);
				ColumnCheckBoxFlags("Tile Box", &pWnd->WindowStyle, CWS_TILEBOX);
				ColumnCheckBoxFlags("Minimize Box", &pWnd->WindowStyle, CWS_MINIMIZE);
				ColumnCheckBoxFlags("Border", &pWnd->WindowStyle, CWS_BORDER);
				ColumnCheckBoxFlags("Relative Rect", &pWnd->WindowStyle, CWS_RELATIVERECT);
				ColumnCheckBoxFlags("Vertical Auto Stretch", &pWnd->WindowStyle, CWS_AUTOSTRETCHV);
				ColumnCheckBoxFlags("Horizontal Auto Stretch", &pWnd->WindowStyle, CWS_AUTOSTRETCH);
				ColumnCheckBoxFlags("Resizable", &pWnd->WindowStyle, CWS_RESIZEALL);
				ColumnCheckBoxFlags("Transparent", &pWnd->WindowStyle, CWS_TRANSPARENT);
				ColumnCheckBoxFlags("Use My Alpha", &pWnd->WindowStyle, CWS_USEMYALPHA);
				ColumnCheckBoxFlags("Docking Enabled", &pWnd->WindowStyle, CWS_DOCKING);
				ColumnCheckBoxFlags("Immediate Tooltip", &pWnd->WindowStyle, CWS_TOOLTIP_NODELAY);
				ColumnCheckBoxFlags("Frame", &pWnd->WindowStyle, CWS_FRAMEWND);
				ColumnCheckBoxFlags("No Hit Test", &pWnd->WindowStyle, CWS_NOHITTEST);
				ColumnCheckBoxFlags("QMark Box", &pWnd->WindowStyle, CWS_QMARK);
				ColumnCheckBoxFlags("Disable Move", &pWnd->WindowStyle, CWS_NOMOVABLE);
				ColumnCheckBoxFlags("Maximize Box", &pWnd->WindowStyle, CWS_MAXIMIZE);
				ColumnCheckBoxFlags("Vertical Auto Scroll", &pWnd->WindowStyle, CWS_AUTOVSCROLL);
				ColumnCheckBoxFlags("Horizontal Auto Scroll", &pWnd->WindowStyle, CWS_AUTOHSCROLL);
				ColumnCheckBoxFlags("Client Movable", &pWnd->WindowStyle, CWS_CLIENTMOVABLE);
				ColumnCheckBoxFlags("Transparent Control", &pWnd->WindowStyle, CWS_TRANSPARENTCONTROL);

				ImGui::TreePop();
			}

			DisplayDrawTemplate("Template", pWnd->DrawTemplate);

			DisplayTextureAnimation("Icon", static_cast<CTextureAnimation*>(pWnd->IconTextureAnim));
			ColumnCXRect("Icon rect", pWnd->IconRect);

			ColumnCheckBox("Visible", &pWnd->dShow);
			ColumnCheckBox("Enabled", &pWnd->Enabled);
			ColumnCheckBox("Minimized", &pWnd->Minimized);
			ColumnCheckBox("Maximized", &pWnd->bMaximized);
			ColumnCheckBox("Maximizable", &pWnd->bMaximizable);
			ColumnCheckBox("Tiled", &pWnd->bTiled);
			ColumnCheckBox("Action", &pWnd->bAction);
			ColumnCheckBox("Bring to top when clicked", &pWnd->bBringToTopWhenClicked);
			ColumnCheckBox("Mouse over", &pWnd->MouseOver);

			// Background
			ColumnText("Background type", XWndBackgroundTypeToString(static_cast<XWndBackgroundType>(pWnd->BGType)));
			ColumnText("Background draw type", XWndBackgroundDrawTypeToString(static_cast<XWndBackgroundDrawType>(pWnd->BackgroundDrawType)));
			ColumnColor("Normal color", pWnd->CRNormal);
			ColumnColor("Background color", pWnd->BGColor);
			ColumnColor("Disabled background color", pWnd->DisabledBackground);

			ColumnCXStr("XML Tooltip", pWnd->XMLToolTip);

			// size
			ColumnCXSize("Min size", pWnd->MinClientSize);
			ColumnCXSize("Max size", pWnd->MaxClientSize);

			// escape-to-close
			ColumnCheckBox("Escapable", &pWnd->CloseOnESC);
			ColumnCheckBox("Escapable locked", &pWnd->bEscapableLocked);

			ColumnText("Horizontal scroll", "{ pos=%d, max=%d }", pWnd->HScrollPos, pWnd->HScrollMax);
			ColumnText("Vertical scroll", "{ pos=%d, max=%d }", pWnd->VScrollPos, pWnd->VScrollMax);

			ColumnCheckBox("Use in horizontal layout", &pWnd->bUseInLayoutHorizontal);
			ColumnCheckBox("Use in vertical layout", &pWnd->bUseInLayoutVertical);
			ColumnText("Anchors", "{ top=%d, right=%d, bottom=%d, left=%d }", pWnd->bTopAnchoredToTop, pWnd->bRightAnchoredToLeft, pWnd->bBottomAnchoredToTop, pWnd->bLeftAnchoredToLeft);
			ColumnText("Offsets", "{ top=%d, right=%d, bottom=%d, left=%d", pWnd->TopOffset, pWnd->RightOffset, pWnd->BottomOffset, pWnd->LeftOffset);

			// Alpha
			ColumnCheckBox("Fade enabled", &pWnd->Fades);
			ColumnText("Current fade alpha", "%d", pWnd->FadeAlpha);
			ColumnText("Current max alpha", "%d", pWnd->Alpha);

			// Mouse over / fading stuff
			ColumnCheckBox("Faded", &pWnd->Faded);
			ColumnText("Last time mouse over", "%d", pWnd->LastTimeMouseOver);
			ColumnText("Fade delay", "%d", pWnd->FadeDelay);
			ColumnText("Fade duration", "%d", pWnd->FadeDuration);
			ColumnText("Fade to alpha", "%d", pWnd->FadeToAlpha);

			// Transition effects
			if (ColumnTreeNode("Transition Properties", ""))
			{
				ColumnText("Start alpha", "%d", pWnd->StartAlpha);
				ColumnText("Target alpha", "%d", pWnd->TargetAlpha);
				ColumnText("Transition start tick", "%d", pWnd->TransitionStartTick);
				ColumnText("Transition duration", "%d", pWnd->TransitionDuration);
				ColumnCheckBox("Is transitioning", &pWnd->bIsTransitioning);
				ColumnText("Transition", "%d", pWnd->Transition);
				ColumnCXRect("Transition rect", pWnd->TransitionRect);

				ImGui::TreePop();
			}

			if (ColumnTreeNode("Blink Properties", ""))
			{
				ColumnText("Blink fade frequency", "%d", pWnd->BlinkFadeFreq);
				ColumnText("Last blink fade refresh time", "%d", pWnd->LastBlinkFadeRefreshTime);
				ColumnText("Blink fade duration", "%d", pWnd->BlinkFadeDuration);
				ColumnText("Blink fade start time", "%d", pWnd->BlinkFadeStartTime);
				ColumnText("Blink state", "%d", pWnd->BlinkState);
				ColumnText("Blink start timer", "%d", pWnd->BlinkStartTimer);
				ColumnText("Blink duration", "%d", pWnd->BlinkDuration);

				ImGui::TreePop();
			}

			ColumnText("Valid", pWnd->ValidCXWnd ? "true" : "false");
			ColumnCheckBox("Unlockable", &pWnd->Unlockable);
			ColumnCheckBox("Keep on screen", &pWnd->bKeepOnScreen);
			ColumnCheckBox("Locked", &pWnd->Locked);
			ColumnCheckBox("Clip to parent", &pWnd->bClipToParent);
			ColumnCheckBox("Clickable", &pWnd->Clickable);
			ColumnCheckBox("Click through", &pWnd->bClickThrough);
			ColumnCheckBox("Show click through menu item", &pWnd->bShowClickThroughMenuItem);
			ColumnCheckBox("Active", &pWnd->bActive);

			//ColumnText("Resizable mask", "0x%08x", pWnd->bResizableMask);
			//ColumnCheckBox("Border", &pWnd->bBorder);
			//ColumnCheckBox("Border 2", &pWnd->bBorder2);
			//ColumnCheckBox("Top anchored to top", &pWnd->bTopAnchoredToTop);
			//ColumnCheckBox("Right anchored to left", &pWnd->bRightAnchoredToLeft);
			//ColumnCheckBox("Bottom anchored to top", &pWnd->bBottomAnchoredToTop);
			//ColumnCheckBox("Left anchored to left", &pWnd->bLeftAnchoredToLeft);
			//ColumnText("Top offset", "%d", pWnd->TopOffset);
			//ColumnText("Right offset", "%d", pWnd->RightOffset);
			//ColumnText("Left offset", "%d", pWnd->LeftOffset);
			//ColumnText("Bottom offset", "%d", pWnd->BottomOffset);
			//ColumnText("Parent/Context Menu array index", "%d", pWnd->ParentAndContextMenuArrayIndex);
			//ColumnCheckBox("Click through menu item", &pWnd->bClickThroughMenuItemStatus);
			//ColumnText("Fully clipped", pWnd->bFullyScreenClipped ? "true" : "false");
			//ColumnCheckBox("Needs saving", &pWnd->bNeedsSaving);
			//ColumnCheckBox("Is parent/context menu window", &pWnd->bIsParentOrContextMenuWindow);
			//ColumnText("Data", "%lld", pWnd->Data);
			//ColumnCXStr("DataStr", pWnd->DataStr);
			//ColumnCXRect("Clip rect screen", pWnd->ClipRectScreen);
			//ColumnText("XML Index", "%d", pWnd->XMLIndex);
			//ColumnCheckBox("Capture title", &pWnd->bCaptureTitle);
			// TextObject
			// RuntimeTypes
			// bClientClipRectChanged
			// ParentWindow
			// pTipTextObject
			// bScreenClipRectChanged
			// FocusProxy
			// pFont
			// OldLocation
			// LayoutStrategy
			// TitlePiece
			//ColumnCXRect("Clip client rect", pWnd->ClipRectClient);

			EndColorSection();
		}
	}

	void DisplayDetailsSection(CSidlScreenWnd* pWnd, bool open = true)
	{
		DisplayDetailsSection(static_cast<CXWnd*>(pWnd), open);

		// Add CSidlScreenWnd specific details here
		if (BeginColorSection("CSidlScreenWnd Properties", open))
		{
			ColumnCXStr("Sidl screen name", pWnd->SidlText);
			DisplayScreenTemplate("Screen template", pWnd->SidlPiece);
			// TODO: RadioGroup
			ColumnText("Ini flags", "0x%08x", pWnd->IniFlags);
			ColumnCXStr("Ini settings name", pWnd->IniStorageName);
			ColumnText("Ini version", "%d", pWnd->IniVersion);
			//ColumnText("Last resolution pos", "{ x=%d, y=%d }", pWnd->LastResX, pWnd->LastResY);
			//ColumnText("Last resolution fullscreen", pWnd->bLastResFullscreen ? "true" : "false");
			//ColumnText("Context menu id", "%d", pWnd->ContextMenuID);
			//ColumnText("Context menu tip id", "%d", pWnd->ContextMenuTipID);

			EndColorSection();
		}
	}

	void DisplayDetailsSection(CButtonWnd* pWnd, bool open = true)
	{
		DisplayDetailsSection(static_cast<CXWnd*>(pWnd), open);

		if (BeginColorSection("CButtonWnd Properties", open))
		{
			DisplayDrawTemplate("Template", pWnd->DrawTemplate);

			ColumnText("Picture button", pWnd->bPicture ? "true" : "false");
			// CRadioGroup
			ColumnCheckBox("Checked", &pWnd->bChecked);
			//ColumnText("Is check box", pWnd->bIsCheckbox ? "true" : "false");

			// bMouseOverLastFrame
			ColumnCXPoint("Decal offset", pWnd->DecalOffset);
			ColumnCXSize("Decal size", pWnd->DecalSize);
			ColumnColor("Decal tint", pWnd->DecalTint);

			ColumnCXRect("Text offsets", pWnd->TextOffsets);

			// TODO: flags for text mode
			ColumnText("Text mode bits", "%d", pWnd->TextModeBits);

			ColumnColor("Hover tint", pWnd->Mouseover);
			ColumnColor("Pressed tint", pWnd->Pressed);
			ColumnColor("Disabled tint", pWnd->Disabled);

			ColumnText("Cooldown time", "%d", pWnd->CoolDownBeginTime);
			ColumnText("Cooldown duration", "%d", pWnd->CoolDownDuration);

			ColumnCXStr("Indicator", pWnd->Indicator);
			ColumnText("Indicator value", "%d", pWnd->IndicatorVal);

			// pIndicatorTextObject
			// bAllowButtonClickThrough
			// bCoolDownDoDelayedStart
			// bIsDrawLasso
			//ColumnText("Button style", "0x%08x", pWnd->ButtonStyle);

			// CLabel

			EndColorSection();
		}
	}

	void DisplayDetailsSection(CSpellGemWnd* pWnd, bool open = true)
	{
		DisplayDetailsSection(static_cast<CButtonWnd*>(pWnd), false);

		if (BeginColorSection("CSpellGemWnd Properties", true))
		{
			DisplayDrawTemplate("Template", pWnd->DrawTemplate);

			DisplayTextureAnimation("Spell icon texture", pWnd->SpellIconTexture);
			DisplayTextureAnimation("Custom icon texture", pWnd->CustomIconTexture);

			EndColorSection();
		}
	}
};

#pragma endregion

//============================================================================
//============================================================================

#pragma region Windows Developer Tool

class ImGuiWindowsDeveloperTool
{
	CXWnd* m_pSelectedWnd = nullptr;
	CXWnd* m_pHoveredWnd = nullptr;
	CXWnd* m_pLastSelected = nullptr;
	bool m_selectionChanged = false;
	float m_topPaneSize = -1.0f;
	float m_bottomPaneSize = -1.0f;
	bool m_picking = false;
	bool m_selectPicking = false;
	CXWnd* m_pPickingWnd = nullptr;
	int m_lastWindowCount = 0;

	ImGuiWindowPropertyViewer m_propertyViewer;

public:
	ImGuiWindowsDeveloperTool() = default;

	void OnWindowRemoved(CXWnd* pWnd)
	{
		// Clear the property viewer if its using this window.
		if (m_propertyViewer.GetWindow() == pWnd)
		{
			m_propertyViewer.SetWindow(nullptr);
		}

		// Clear any matching selections in the window.
		if (m_pSelectedWnd == pWnd)
			m_pSelectedWnd = nullptr;
		if (m_pHoveredWnd == pWnd)
			m_pHoveredWnd = nullptr;
		if (m_pPickingWnd == pWnd)
			m_pPickingWnd = nullptr;
		if (m_pLastSelected == pWnd)
			m_pLastSelected = nullptr;
	}

	void Reset()
	{
		m_propertyViewer.SetWindow(nullptr);
		m_pSelectedWnd = nullptr;
		m_pHoveredWnd = nullptr;
		m_pPickingWnd = nullptr;
		m_pLastSelected = nullptr;
		m_picking = false;
		m_selectPicking = false;
	}

	void Draw()
	{
		m_pHoveredWnd = nullptr;
		m_pPickingWnd = nullptr;

		ImVec2 availSize = ImGui::GetContentRegionAvail();
		if (m_topPaneSize == -1.0f)
			m_topPaneSize = std::min(250.f, availSize.y * .55f);
		if (m_bottomPaneSize == -1.0f)
			m_bottomPaneSize = availSize.y - m_topPaneSize - 1;

		if (m_picking)
		{
			m_pPickingWnd = pWndMgr->LastMouseOver;
		}

		if (m_picking && m_selectPicking)
		{
			m_selectPicking = false;
			m_picking = false;

			m_pSelectedWnd = m_pPickingWnd;
			m_pPickingWnd = nullptr;
		}

		if (ImGui::Button("Pick"))
		{
			m_picking = !m_picking;
		}

		if (m_picking)
		{
			ImGui::SameLine();
			ImGui::Text("Pick:");
			ImGui::SameLine();

			if (m_pPickingWnd)
			{
				ImGui::TextColored(ImColor(0.f, 1.0f, 0.0f, 1.0f), "%s", m_pPickingWnd->GetXMLName().c_str());
			}
			else
			{
				ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, 0.5f), "(none)");
			}
		}

		if (m_lastWindowCount != 0)
		{
			ImGui::SameLine();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%d Windows", m_lastWindowCount);
		}

		imgui::DrawSplitter(true, 9.0f, &m_topPaneSize, &m_bottomPaneSize, 100, 100);

		DisplayWindowTree();

		// update last selected to remember selection for next iteration
		m_selectionChanged = test_and_set(m_pLastSelected, m_pSelectedWnd);

		if (ImGui::BeginChild("##InspectPanel"))
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 9);

			m_propertyViewer.SetWindow(m_pSelectedWnd);
			m_propertyViewer.Draw();
		}

		ImGui::EndChild();

		// Update background overlay showing whats currently highlighted or selected
		DrawBackgroundWindowHighlights();
	}

	void DrawBackgroundWindowHighlights()
	{
		if (m_pSelectedWnd)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);

			CXRect clientRect = m_pSelectedWnd->GetClientRect();
			drawList->AddRect(
				ImVec2(clientRect.left + viewport->Pos.x, clientRect.top + viewport->Pos.y),
				ImVec2(clientRect.right + viewport->Pos.x, clientRect.bottom + viewport->Pos.y),
				IM_COL32(124, 252, 0, 200));
		}

		if (m_pPickingWnd)
		{
			CXWnd* wnd = m_pPickingWnd ? m_pPickingWnd : m_pSelectedWnd;

			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);

			CXRect clientRect = wnd->GetClientRect();
			drawList->AddRectFilled(
				ImVec2(clientRect.left + viewport->Pos.x, clientRect.top + viewport->Pos.y),
				ImVec2(clientRect.right + viewport->Pos.x, clientRect.bottom + viewport->Pos.y),
				IM_COL32(138, 179, 191, 128));
		}

		if (m_pHoveredWnd)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);

			CXRect clientRect = m_pHoveredWnd->GetClientRect();
			drawList->AddRect(
				ImVec2(clientRect.left + viewport->Pos.x, clientRect.top + viewport->Pos.y),
				ImVec2(clientRect.right + viewport->Pos.x, clientRect.bottom + viewport->Pos.y),
				m_pHoveredWnd->IsReallyVisible() ? IM_COL32(255, 215, 0, 200) : IM_COL32(200, 20, 60, 200));
		}
	}

	void DisplayWindowTree()
	{
		ImGuiTableFlags tableFlags = ImGuiTableFlags_ScrollFreezeTopRow | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersHOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;

		if (ImGui::BeginTable("##WindowTable", 2, tableFlags, ImVec2(0, m_topPaneSize)))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
			ImGui::TableAutoHeaders();

			m_lastWindowCount = 0;
			if (pWndMgr)
			{
				for (CXWnd* pWnd : pWndMgr->ParentAndContextMenuWindows)
				{
					if (pWnd->ParentWindow == nullptr)
					{
						DisplayWindowTreeNode(pWnd);
						++m_lastWindowCount;
					}
				}
			}

			ImGui::EndTable();
		}
	}

	void DisplayWindowTreeNode(CXWnd* pWnd)
	{
		if (pWnd->GetType() == UI_Unknown)
			return;
		ImGui::TableNextRow();
		const bool hasChildren = pWnd->GetFirstChildWnd() != nullptr;

		CXStr pName = pWnd->GetXMLName();
		CXMLData* pXMLData = pWnd->GetXMLData();
		CXStr typeName = pWnd->GetTypeName();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool open = false;
		bool selected = (m_pLastSelected == pWnd);
		bool selectPicking = false;

		if (m_picking)
		{
			if (m_pPickingWnd == pWnd)
			{
				selected = true;
				selectPicking = true;
			}
		}

		if (selected)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		if (!hasChildren)
		{
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		if (hasChildren)
		{
			if (m_pPickingWnd)
			{
				if (m_pPickingWnd->IsDescendantOf(pWnd))
					flags |= ImGuiTreeNodeFlags_DefaultOpen;
			}
			else if (m_pSelectedWnd && m_selectionChanged)
			{
				if (m_pSelectedWnd->IsDescendantOf(pWnd))
				{
					ImGui::SetNextItemOpen(true);
				}
			}

			open = ImGui::TreeNodeEx(pWnd, flags, "%s", pName.c_str());
		}
		else
		{
			ImGui::TreeNodeEx(pWnd, flags, "%s", pName.c_str());
		}

		if (selectPicking)
		{
			ImGui::SetScrollHere();
		}

		if (ImGui::IsItemHovered())
		{
			m_pHoveredWnd = pWnd;
		}

		if (ImGui::IsItemClicked())
		{
			m_pSelectedWnd = pWnd;
		}

		ImGui::TableNextCell();
		ImGui::Text("%s", typeName.c_str());

		if (open)
		{
			CXWnd* pChild = pWnd->GetFirstChildWnd();
			while (pChild)
			{
				DisplayWindowTreeNode(pChild);
				pChild = pChild->GetNextSiblingWnd();
			}

			// If this is a list box, then also traverse its child list windows.
			if (pWnd->GetType() == UI_Listbox)
			{
				CListWnd* listWnd = static_cast<CListWnd*>(pWnd);

				for (SListWndLine& line : listWnd->ItemsArray)
				{
					// TODO: Expand into columns/rows but for now just list the children.
					for (SListWndCell& cell : line.Cells)
					{
						// The children of the list are stored in wrapper windows. They show up
						// as Unknown types. We just skip past them.
						if (cell.pWnd && cell.pWnd->GetFirstChildWnd())
						{
							DisplayWindowTreeNode(cell.pWnd->GetFirstChildWnd());
						}
					}
				}
			}

			ImGui::TreePop();
		}
	}

	bool IsPicking() const { return m_picking; }

	void Pick()
	{
		if (m_picking)
			m_selectPicking = true;
	}
};

#pragma endregion

static ImGuiWindowsDeveloperTool s_windowDebugPanel;

bool DeveloperTools_HandleClick(int mouseButton, bool clicked)
{
	// If the picker is active, tell it that we clicked. Returns true (to consume the click) if this happens.
	if (mouseButton == 0 && clicked && s_windowDebugPanel.IsPicking())
	{
		s_windowDebugPanel.Pick();
		return true;
	}

	return false;
}

void DeveloperTools_RemoveWindow(CXWnd* pWnd)
{
	s_windowDebugPanel.OnWindowRemoved(pWnd);
}

void DeveloperTools_CloseLoginFrontend()
{
	s_windowDebugPanel.Reset();
}

//============================================================================

void DeveloperTools_Initialize()
{
	AddDebugPanel("Windows", []() { s_windowDebugPanel.Draw(); });
}

void DeveloperTools_Shutdown()
{
	RemoveDebugPanel("Windows");
}

void DeveloperTools_SetGameState(DWORD gameState)
{
	s_windowDebugPanel.Reset();
}

} // namespace mq
