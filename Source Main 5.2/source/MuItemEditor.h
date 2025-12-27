#pragma once

#ifdef _EDITOR

#include <string>

class CMuItemEditor
{
public:
    static CMuItemEditor& GetInstance();

    void Render(bool& showEditor);
    void ClearSearch() { m_szItemSearchBuffer[0] = '\0'; }

    int GetSelectedItemIndex() const { return m_iSelectedItemIndex; }

private:
    CMuItemEditor();
    ~CMuItemEditor() = default;

    void RenderSearchBar();
    void RenderItemTable(const std::string& searchLower);
    void RenderSaveButton();

    char m_szItemSearchBuffer[256];
    int m_iSelectedItemIndex;
};

#define g_MuItemEditor CMuItemEditor::GetInstance()

#endif // _EDITOR
