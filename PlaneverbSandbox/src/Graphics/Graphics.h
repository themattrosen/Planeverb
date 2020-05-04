//Matthew Rosen
#pragma once

#include <Util.h>

struct GLFWwindow;

class Graphics : public Singleton<Graphics>
{
public:
	void Init();
	void Update();
	void Draw();
	void Exit();

	GLFWwindow* GetWindow() { return m_window; }
	bool IsRunning() const;

private:
	GLFWwindow* m_window;
};