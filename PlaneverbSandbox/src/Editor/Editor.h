//Matthew Rosen
#pragma once

#include "Util.h"
#include "Audio\AudioData.h"
#include <Planeverb.h>
#include "ImGui\imgui.h"

#include <unordered_map>
#include <string>

struct GLFWwindow;
struct ImDrawList;

class Editor : public Singleton<Editor>
{
public:
	void Init(GLFWwindow* window);
	void Update();
	void Exit();

	Editor() {}

	const Planeverb::vec3& GetEmitter() const { return m_emitter; }

private:

	struct
	{
		bool demoWindow;
		bool geometryWindow;
		bool audioWindow;
		bool gridWindow;
		bool analyzerWindow;
	} m_windowFlags;

	void InternalUpdate();
	void WindowUpdate();
	void MenuUpdate();
	void InternalDraw();

	void ShowGeometryWindow();
	void SaveGeometry(const char* filename);
	void LoadGeometry(const char* filename);
	void ShowAudioWindow();
	void ShowGridWindow();
	void ShowAnalyzerWindow();

	void DisplayGrid(ImDrawList* drawList);
	void DisplayAABBs(ImDrawList* drawList, const ImVec2& offset);
	void DisplayEmitterAndListener(ImDrawList* drawList, const ImVec2& offset);

	std::unordered_map<Planeverb::PlaneObjectID, Planeverb::AABB> m_geometry;
	Planeverb::vec3 m_emitter;
	Planeverb::EmissionID m_emitterID = Planeverb::PV_INVALID_EMISSION_ID;
	Planeverb::vec3 m_listener;
	std::string m_currentFile;
	bool m_usePlaneverb;
	float m_volumeDB;
	AudioData m_audioData;

	// grid stuff
	bool m_showGrid = true;
	ImVec2 m_scrolling;
	Planeverb::PlaneObjectID m_itemHovered;
	Planeverb::PlaneObjectID m_itemSelected;
};
