#pragma once

#ifdef _EDITOR

#include <string>

class CItemSetOptionEditorTable
{
public:
    CItemSetOptionEditorTable() = default;
    ~CItemSetOptionEditorTable() = default;

    void Render(const std::string& searchFilter, int& selectedRow, bool showEmpty);

private:
    void RenderRow(int setIndex, bool& rowInteracted);
};

#endif // _EDITOR
