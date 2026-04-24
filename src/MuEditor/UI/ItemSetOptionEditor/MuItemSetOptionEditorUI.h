#pragma once

#ifdef _EDITOR

class CItemSetOptionEditorTable;

class CMuItemSetOptionEditorUI
{
public:
    static CMuItemSetOptionEditorUI& GetInstance();

    void Render(bool& showEditor);
    void ClearSearch() { m_szSearchBuffer[0] = '\0'; }

private:
    CMuItemSetOptionEditorUI();
    ~CMuItemSetOptionEditorUI();

    CMuItemSetOptionEditorUI(const CMuItemSetOptionEditorUI&) = delete;
    CMuItemSetOptionEditorUI& operator=(const CMuItemSetOptionEditorUI&) = delete;

    void RenderSearchBar();

    char m_szSearchBuffer[256];
    int  m_selectedRow;
    bool m_bShowEmpty;
    CItemSetOptionEditorTable* m_pTable;
};

#define g_MuItemSetOptionEditorUI CMuItemSetOptionEditorUI::GetInstance()

#endif // _EDITOR
