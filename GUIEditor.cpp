#include "GUIEditor.h"
#include "DxLib.h"
#include "GUI.h"

namespace GUIEditor {

void DrawInspectorPanel(std::shared_ptr<GameObject> obj) {
    DrawInspector(obj);
}

} // namespace GUIEditor
