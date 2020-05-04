//Matthew Rosen
#include "Editor.h"
#include "ImGui\imgui.h"
#include "ImGui\imgui_impl_glfw.h"
#include "ImGui\imgui_impl_opengl3.h"
#include "glfw3.h"
#include "Audio\AudioCore.h"
#include "WindowsFileBrowsing.h"
#include <PlaneverbDSP.h>
#include <string>
#include <filesystem>

void Editor::Init(GLFWwindow * window)
{
	const char* glsl_version = "#version 130";
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	//todo
	m_windowFlags.demoWindow = false;
	m_windowFlags.geometryWindow = true;
	m_windowFlags.audioWindow = true;
	m_windowFlags.gridWindow = true;
	m_windowFlags.analyzerWindow = true;
	
	m_listener = Planeverb::vec3(5.f, 0.f, 4.f);
	m_emitter = Planeverb::vec3(5.f, 0.f, 6.f);
	Planeverb::SetListenerPosition(m_listener);
	PlaneverbDSP::SetListenerTransform(m_listener.x, m_listener.y, m_listener.z,
		0, 0, 1);
}

void Editor::Update()
{
	InternalUpdate();

	MenuUpdate();
	WindowUpdate();

	InternalDraw();
}

void Editor::Exit()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Editor::InternalUpdate()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Editor::WindowUpdate()
{
	if(m_windowFlags.demoWindow)
		ImGui::ShowDemoWindow(&m_windowFlags.demoWindow);
	ShowGeometryWindow();
	ShowAudioWindow();
	ShowGridWindow();
	ShowAnalyzerWindow();
}

void Editor::MenuUpdate()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Windows"))
		{
			ImGui::MenuItem("Geometry Editor", NULL, &m_windowFlags.geometryWindow);
			ImGui::MenuItem("Transport Window", NULL, &m_windowFlags.audioWindow);
			ImGui::MenuItem("Grid Viewer", NULL, &m_windowFlags.gridWindow);
			ImGui::MenuItem("Analyzer Window", NULL, &m_windowFlags.analyzerWindow);
			
			ImGui::Separator();

			ImGui::MenuItem("ImGui Demo Window", NULL, &m_windowFlags.demoWindow);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void Editor::InternalDraw()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::ShowGeometryWindow()
{
	if (!m_windowFlags.geometryWindow)
		return;

	if (ImGui::Begin("Geometry Editor", &m_windowFlags.geometryWindow))
	{
		int i = 0;
		for (auto& pair : m_geometry)
		{
			auto& ab = pair.second;
			ImGui::Text("ID: %d", pair.first);
			ImGui::SameLine();

			std::string minusButton = "-##" + std::to_string(i);
			if (ImGui::Button(minusButton.c_str()))
			{
				Planeverb::RemoveGeometry(pair.first);
				m_geometry.erase(pair.first);
				break;
			}

			ImGui::SameLine();

			std::string buttonName = "Edit##" + std::to_string(i);
			std::string popupName = "EditAABB##" + std::to_string(i);
			if (ImGui::Button(buttonName.c_str()))
			{
				ImGui::OpenPopup(popupName.c_str());
			}
			ImGui::Text("Position: (%.4f, %.4f)",
				ab.position.x, ab.position.y);
			ImGui::Text("Size: (%.4f, %.4f)",
				ab.width, ab.height);
			ImGui::Text("Absorption: %.4f", ab.absorption);
		
			if (ImGui::BeginPopup(popupName.c_str()))
			{
				ImGui::DragFloat2("Position", (float*)ab.position.m, 0.01f);
				ImGui::DragFloat("Width", (float*)&ab.width, 0.01f);
				ImGui::DragFloat("Height", (float*)&ab.height, 0.01f);
				ImGui::DragFloat("Absorption", (float*)&ab.absorption, 0.01f);

				std::string commitName = "Update##" + std::to_string(i);
				if (ImGui::Button(commitName.c_str()))
				{
					Planeverb::UpdateGeometry(pair.first, &ab);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			++i;
			ImGui::Separator();
		}

		ImGui::Separator();
		if (ImGui::Button("Save", ImVec2(0, 25)))
		{
			std::string filename;
			bool result = SaveOrOpenFile(filename, "pv", "Planeverb File\0*.pv\0Any File\0*.*\0\0", true);
			if (result)
			{
				this->SaveGeometry(filename.c_str());
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Load", ImVec2(0, 25)))
		{
			std::string filename;
			bool result = SaveOrOpenFile(filename, "pv", "Planeverb File\0*.pv\0Any File\0*.*\0\0", false);
			if (result)
			{
				this->LoadGeometry(filename.c_str());
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("+", ImVec2(25, 25)))
		{
			ImGui::OpenPopup("AddAABB");
		}

		if (ImGui::BeginPopup("AddAABB"))
		{
			static Planeverb::AABB ab;
			ImGui::DragFloat2("Position", (float*)ab.position.m, 0.01f);
			ImGui::DragFloat("Width", (float*)&ab.width, 0.01f);
			ImGui::DragFloat("Height", (float*)&ab.height, 0.01f);
			ImGui::DragFloat("Absorption", (float*)&ab.absorption, 0.01f);

			std::string commitName = "Add##" + std::to_string(i);
			if (ImGui::Button(commitName.c_str()))
			{
				auto id = Planeverb::AddGeometry(&ab);
				m_geometry.insert_or_assign(id, ab);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}
}

void Editor::SaveGeometry(const char * filename)
{
	std::ofstream stream(filename);
	if (!stream.is_open())
	{
		return;
	}

	// serialize size of map
	size_t size = m_geometry.size();
	stream << size << std::endl;

	// serialize all pairs in map
	for (auto it = m_geometry.begin(); it != m_geometry.end(); ++it)
	{
		stream << it->first << " ";
		stream << it->second.position.x << " ";
		stream << it->second.position.y << " ";
		stream << it->second.width << " ";
		stream << it->second.height << " ";
		stream << it->second.absorption << std::endl;
	}

	stream.close();
}

void Editor::LoadGeometry(const char * filename)
{
	std::ifstream stream(filename);
	if (!stream.is_open())
	{
		return;
	}

	// call remove on all pairs in map
	for (auto& pair : m_geometry)
	{
		Planeverb::RemoveGeometry(pair.first);
	}

	// clear map
	m_geometry.clear();

	// read in size of new map
	size_t size = 0;
	stream >> size;

	// insert each element into the new map
	for (size_t i = 0; i < size; ++i)
	{
		Planeverb::AABB next;
		Planeverb::PlaneObjectID id;
		stream >> id;
		stream >> next.position.x;
		stream >> next.position.y;
		stream >> next.width;
		stream >> next.height;
		stream >> next.absorption;

		id = Planeverb::AddGeometry(&next);
		m_geometry.insert_or_assign(id, next);
	}
}

void Editor::ShowAudioWindow()
{
	if (!m_windowFlags.audioWindow)
		return;

	if (ImGui::Begin("Transport Window", &m_windowFlags.audioWindow))
	{
		ImGui::Text("Current File: %s", m_currentFile.c_str());
		ImGui::Separator();
		if (ImGui::Button("Play", ImVec2(50, 25)))
		{
			m_emitterID = AudioCore::Instance().PlayAudio(m_emitter);
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop", ImVec2(50, 25)))
		{
			m_emitterID = Planeverb::PV_INVALID_EMISSION_ID;
			AudioCore::Instance().StopAudio();
		}

		if (ImGui::Checkbox("Use Planeverb", &m_usePlaneverb))
		{
			AudioCore::Instance().SetUsePlaneverb(m_usePlaneverb);
		}
		ImGui::Separator();

		if (ImGui::SliderFloat("Volume (dB)", &m_volumeDB, -40.f, 12.f))
		{
			AudioCore::Instance().SetVolume(dBToGain(m_volumeDB));
		}
		ImGui::Separator();
		if (ImGui::Button("Import", ImVec2(ImGui::GetWindowWidth(), 0)))
		{
			bool result = SaveOrOpenFile(m_currentFile, "wav", "WAV File\0*.wav\0All Files\0*.*\0\0", false);
			if (result)
			{
				AudioData::read_wave(m_currentFile.c_str(), m_audioData);

				AudioCore::Instance().SetAudioData(&m_audioData);

				namespace fs = std::experimental::filesystem;
				fs::path p(m_currentFile);
				m_currentFile = p.filename().string();
			}
		}


		ImGui::End();
	}
}

ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

ImVec2 operator*(const ImVec2& lhs, const float& rhs)
{
	return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

void Editor::ShowGridWindow()
{
	if (!m_windowFlags.gridWindow)
		return;

	if (ImGui::Begin("Grid Viewer", &(m_windowFlags.gridWindow)))
	{
		ImGui::BeginGroup();

		ImGui::Checkbox("Show Grid", &m_showGrid);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
		ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
		ImGui::PushItemWidth(120.f);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 offset = ImGui::GetCursorScreenPos() + m_scrolling;
		DisplayGrid(drawList);
		DisplayAABBs(drawList, offset);
		DisplayEmitterAndListener(drawList, offset);

		if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
			(ImGui::IsMouseDragging(1, 0.f) || ImGui::IsMouseDragging(2, 0.f)))
		{
			m_scrolling = m_scrolling + ImGui::GetIO().MouseDelta;
		}

		ImGui::PopItemWidth();
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::EndGroup();
		ImGui::End();
	}
}

void Editor::ShowAnalyzerWindow()
{
	if (!m_windowFlags.analyzerWindow)
		return;

	if (ImGui::Begin("Analyzer Window", &m_windowFlags.analyzerWindow))
	{
		
		Planeverb::PlaneverbOutput result;
		result.occlusion = Planeverb::PV_INVALID_DRY_GAIN;

		if(m_emitterID != Planeverb::PV_INVALID_EMISSION_ID)
			result = Planeverb::GetOutput(m_emitterID);
		
		ImGui::Text("Analyzer Result for Emitter Position (%f, %f)", m_emitter.x, m_emitter.z);
		ImGui::Separator();

		if (result.occlusion == Planeverb::PV_INVALID_DRY_GAIN)
		{
			ImGui::Text("Dry Gain  : ");
			ImGui::Text("Lowpass   : ");
			ImGui::Text("RT60      : ");
			ImGui::Text("Direction : ");
		}
		else
		{
			ImGui::Text("Dry Gain  : %f", result.occlusion);
			ImGui::Text("Lowpass   : %f", result.lowpass);
			ImGui::Text("RT60      : %f", result.rt60);
			ImGui::Text("Direction : (%f, %f)", result.direction.x, result.direction.y);
		}

		ImGui::End();
	}
}

void Editor::DisplayGrid(ImDrawList * list)
{
	if (!m_showGrid) return;

	static const ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
	static const float GRID_SZ = 64.f;
	ImVec2 winPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSz = ImGui::GetWindowSize();
	for (float x = std::fmod(m_scrolling.x, GRID_SZ); x < canvasSz.x; x += GRID_SZ)
	{
		list->AddLine(ImVec2(x, 0.f) + winPos, ImVec2(x, canvasSz.y) + winPos, GRID_COLOR);
	}
	for (float y = std::fmod(m_scrolling.y, GRID_SZ); y < canvasSz.y; y += GRID_SZ)
	{
		list->AddLine(ImVec2(0.f, y) + winPos, ImVec2(canvasSz.x, y) + winPos, GRID_COLOR);
	}
}

static const float GRID_TO_WORLD_SCALE = 64.f;

void Editor::DisplayAABBs(ImDrawList * drawList, const ImVec2 & offset)
{
	static const ImVec2 NODE_WINDOW_PADDING{ 8.f, 8.f };
	static const ImU32 NODE_BORDER_COLOR = IM_COL32(200, 200, 100, 255);

	drawList->AddRect(ImVec2(0, 0) + offset, (ImVec2(10, 10) * GRID_TO_WORLD_SCALE) + offset, NODE_BORDER_COLOR);

	for (auto& pair : m_geometry)
	{
		ImGui::PushID((int)pair.first);

		ImVec2 pos(pair.second.position.x, pair.second.position.y);
		pos = pos * GRID_TO_WORLD_SCALE;
		float width = pair.second.width;
		float height = pair.second.height;
		ImVec2 size(width, height);

		//TODO: scale size by something to actually make them show up bigly
		size = size * GRID_TO_WORLD_SCALE;

		ImVec2 rectMin = offset + pos;
		rectMin.x -= size.x / 2.f;
		rectMin.y -= size.y / 2.f;

		bool nodeWidgetsActive = ImGui::IsAnyItemActive();

		ImGui::SetCursorScreenPos(rectMin);
		//ImGui::InvisibleButton("node", size);
		//bool nodeHovered = ImGui::IsItemHovered();
		//if (nodeHovered)
		//{
		//	m_itemHovered = pair.first;
		//}
		bool nodeMovingActive = ImGui::IsItemActive();
		if (nodeWidgetsActive || nodeMovingActive)
		{
			m_itemSelected = pair.first;
		}

		bool isSelected = (m_itemHovered == pair.first ||
			(m_itemHovered != Planeverb::PV_INVALID_PLANE_OBJECT_ID && m_itemSelected == pair.first));

		static const ImU32 NODE_BORDER_COLOR = IM_COL32(200, 200, 100, 255);
		static const ImU32 NODE_HOVERED_BG_COLOR = IM_COL32(75, 75, 75, 255);
		static const ImU32 NODE_DEFAULT_BG_COLOR = IM_COL32(60, 60, 60, 255);

		ImVec2 rectMax = rectMin + size;

		if (isSelected)
		{
			drawList->AddRectFilled(rectMin, rectMax, NODE_HOVERED_BG_COLOR);
		}
		else
		{
			drawList->AddRectFilled(rectMin, rectMax, NODE_DEFAULT_BG_COLOR);
		}

		drawList->AddRect(rectMin, rectMax, NODE_BORDER_COLOR);

		ImGui::PopID();
	}
}

void Editor::DisplayEmitterAndListener(ImDrawList * drawList, const ImVec2 & offset)
{
	static const ImVec2 BUTTON_SIZE(25.f, 25.f);
	static const ImU32 HOVERED_COLOR = IM_COL32(0, 255, 0, 150);
	static const ImU32 NORMAL_COLOR = IM_COL32(25, 100, 25, 150);
	static const ImU32 LISTENER_COLOR = IM_COL32(100, 25, 25, 150);
	static const ImU32 LISTENER_HOVERED = IM_COL32(255, 0, 0, 150);
	static const float BUTTON_RADIUS = 10.f;

	ImVec2 listenerPos = offset + (ImVec2(m_listener.x, m_listener.z) * GRID_TO_WORLD_SCALE);
	ImVec2 listenerRectMin = listenerPos - (BUTTON_SIZE * 0.5f);
	ImVec2 emitterPos = offset + (ImVec2(m_emitter.x, m_emitter.z) * GRID_TO_WORLD_SCALE);
	ImVec2 emitterRectMin = emitterPos - (BUTTON_SIZE * 0.5f);

	ImGui::SetCursorScreenPos(listenerRectMin);
	ImGui::InvisibleButton("listener", BUTTON_SIZE);
	bool isHovered = ImGui::IsItemHovered();
	if (isHovered)
		drawList->AddCircleFilled(listenerPos, BUTTON_RADIUS, LISTENER_HOVERED);
	else
		drawList->AddCircleFilled(listenerPos, BUTTON_RADIUS, LISTENER_COLOR);

	if (isHovered && ImGui::IsMouseDragging())
	{
		ImVec2 delta = ImGui::GetMouseDragDelta();
		ImGui::ResetMouseDragDelta();
		m_listener.x += delta.x / GRID_TO_WORLD_SCALE;
		m_listener.z += delta.y / GRID_TO_WORLD_SCALE;
		Planeverb::SetListenerPosition(m_listener);
		PlaneverbDSP::SetListenerTransform(m_listener.x, m_listener.y, m_listener.z,
			0, 0, 1);
	}

	ImGui::SetCursorScreenPos(emitterRectMin);
	ImGui::InvisibleButton("emitter", BUTTON_SIZE);
	isHovered = ImGui::IsItemHovered();
	if (isHovered)
		drawList->AddCircleFilled(emitterPos, BUTTON_RADIUS, HOVERED_COLOR);
	else
		drawList->AddCircleFilled(emitterPos, BUTTON_RADIUS, NORMAL_COLOR);

	if (isHovered && ImGui::IsMouseDragging())
	{
		ImVec2 delta = ImGui::GetMouseDragDelta();
		ImGui::ResetMouseDragDelta();
		m_emitter.x += delta.x / GRID_TO_WORLD_SCALE;
		m_emitter.z += delta.y / GRID_TO_WORLD_SCALE;
		Planeverb::UpdateEmission(m_emitterID, m_emitter);
	}

	if (m_emitterID != Planeverb::PV_INVALID_EMISSION_ID)
	{
		auto output = Planeverb::GetOutput(m_emitterID);
		drawList->AddLine(listenerPos, listenerPos + (ImVec2(output.direction.x, output.direction.y) * 50), HOVERED_COLOR);
	}
}
