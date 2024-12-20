#include "content_browser.hpp"

#include "engine.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "import/assimp_import.hpp"
#include "import/image_import.hpp"
#include "scene/scene.hpp"

#include <imgui.h>
#include <ranges>
#include <nfd.h>

ContentBrowser::ContentBrowser(const std::string& name, Eng::AssetRegistry& asset_registry, std::shared_ptr<Eng::Scene> in_scene) : UiWindow(name), registry(&asset_registry), scene(in_scene)
{
    memset(filter.data(), 0, filter.size());
}

static std::optional<std::filesystem::path> get_file(const std::vector<std::string>& extensions)
{
    NFD_Init();

    nfdu8char_t*          outPath;

    std::string exts;
    for (const auto& ext : extensions)
        exts += ext + ", ";

    std::vector<nfdu8filteritem_t> filter_items;
    filter_items.emplace_back("available extensions", exts.c_str());


    nfdopendialogu8args_t args = {0};
    args.filterList            = filter_items.data();
    args.filterCount           = static_cast<nfdfiltersize_t>(filter_items.size());
    nfdresult_t result         = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        std::filesystem::path out_path = outPath;
        NFD_FreePathU8(outPath);
        LOG_WARNING("Loading asset {}", out_path.string());
        NFD_Quit();
        return out_path;
    }

    if (result == NFD_CANCEL)
        LOG_WARNING("Item selection canceled");
    else
        LOG_ERROR("Failed to select item : {}", NFD_GetError());

    NFD_Quit();
    return {};
}

void ContentBrowser::draw(Eng::Gfx::ImGuiWrapper& ctx)
{
    internal_draw_id = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 7});
    if (ImGui::Button("Import"))
    {
        ImGui::OpenPopup("ImportPopup");
    }

    if (ImGui::BeginPopup("ImportPopup"))
    {
        if (ImGui::MenuItem("Mesh"))
        {
            if (auto path = get_file({"gltf", "fbx", "obj", "glb", "dae"}))
            {
                auto scene_cp = scene;
                Eng::Engine::get().jobs().schedule(
                    [scene_cp, path]
                    {
                        Eng::AssimpImporter importer;
                        scene_cp->merge(importer.load_from_path(*path));
                    });
            }
        }
        if (ImGui::MenuItem("Image"))
        {
            if (auto path = get_file({"png", "jpg", "dds", "tif", "jpeg", "bmp"}))
            {
                Eng::Engine::get().jobs().schedule(
                    [path]
                    {
                        Eng::ImageImport::load_from_path(*path);
                    });
            }
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Save All"))
        LOG_ERROR("Not implemented yet");

    ImGui::PopStyleVar();
    ImGui::Separator();

    float window_size = std::max(ImGui::GetContentRegionAvail().x / 7.f, 150.f);
    ImGui::Columns(2);
    if (b_set_column_width && ImGui::GetContentRegionAvail().x > 0)
    {
        ImGui::SetColumnWidth(0, window_size);
        b_set_column_width = false;
    }
    drawHierarchy();

    ImGui::NextColumn();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 3});
    ImGui::InputText("##searchBox", filter.data(), filter.size());

    // drawFilters();
    ImGui::Dummy({0, 5});

    ImGui::PopStyleVar();

    if (ImGui::BeginChild("contentAssets"))
    {
        float sizeX = ImGui::GetContentRegionAvail().x;

        int widthItems = static_cast<int>(sizeX / 70);
        ImGui::Columns(std::max(widthItems, 1), "", false);

        registry->for_each(
            [this, &ctx](const TObjectPtr<Eng::AssetBase>& asset)
            {
                draw_asset_thumbnail(asset, ctx);
                ImGui::NextColumn();
            });
        ImGui::Columns(1);
    }
    ImGui::EndChild();
    ImGui::Columns(1);
}

void ContentBrowser::drawHierarchy()
{
    // Initialize left side size
    if (ImGui::BeginChild("folders"))
        drawHierarchy("./resources");
    ImGui::EndChild();
}

void ContentBrowser::drawHierarchy(const std::filesystem::path& f)
{
    if (!exists(f))
        return;
    int  flags           = ImGuiTreeNodeFlags_OpenOnDoubleClick;
    bool bHasFolderChild = false;

    for (auto const& dir_entry : std::filesystem::directory_iterator{f})
    {
        if (dir_entry.is_directory())
            for (auto const& child : std::filesystem::directory_iterator{dir_entry})
            {
                if (child.is_directory())
                {
                    bHasFolderChild = true;
                    break;
                }
            }
    }
    if (!bHasFolderChild)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (selected_file == f)
        flags |= ImGuiTreeNodeFlags_Selected;
    bool bExpand = ImGui::TreeNodeEx(f.filename().string().c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        selected_file    = f;
        show_all_content = false;
    }
    if (bExpand)
    {
        for (auto const& child : std::filesystem::directory_iterator{f})
        {
            if (child.is_directory())
                drawHierarchy(child);
        }
        ImGui::TreePop();
    }
}

static void TextCentered(const std::string& text)
{
    float  avail         = ImGui::GetContentRegionAvail().x;
    ImVec2 text_size     = ImGui::CalcTextSize(text.c_str());
    float  char_width    = text_size.x / text.size();
    int    char_per_line = std::max(1, static_cast<int>(avail / char_width));

    auto  dl            = ImGui::GetWindowDrawList();
    float start_x       = ImGui::GetCursorScreenPos().x;
    int   current_width = 0;
    while (current_width < text.size())
    {
        const char* begin = &text[std::min(current_width, static_cast<int>(text.size()))];
        current_width += char_per_line;
        const char* end = &text[std::min(current_width, static_cast<int>(text.size()))];

        ImGui::SetCursorScreenPos({start_x + (avail - static_cast<float>(end - begin) * char_width) / 2, ImGui::GetCursorScreenPos().y});
        dl->AddText(ImGui::GetCursorScreenPos(), 0xFFFFFFFF, begin, end);

        ImGui::SetCursorScreenPos({start_x, ImGui::GetCursorScreenPos().y + text_size.y});
    }
}

void ContentBrowser::draw_asset_thumbnail(const TObjectPtr<Eng::AssetBase>& asset, Eng::Gfx::ImGuiWrapper& ctx)
{
    if (!asset)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
    ImGui::BeginGroup();
    draw_asset_button(asset, ctx);
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("DDOP_ASSET", asset->get_name(), std::string(asset->get_name()).size());
        draw_asset_button(asset, ctx);
        ImGui::EndDragDropSource();
    }
    ImGui::GetWindowDrawList()->AddRectFilled({ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y - 4}, {ImGui::GetCursorScreenPos().x + 60, ImGui::GetCursorScreenPos().y + 1},
                                              ImGui::ColorConvertFloat4ToU32(ImVec4{asset->asset_color().x, asset->asset_color().y, asset->asset_color().z, 1}));
    ImGui::SetCursorScreenPos({ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 1});
    TextCentered(asset->get_name());
    ImGui::EndGroup();
    ImGui::PopStyleVar();
}

void ContentBrowser::draw_asset_button(const TObjectPtr<Eng::AssetBase>& asset, Eng::Gfx::ImGuiWrapper& ctx)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.2f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});

    if (asset->get_thumbnail())
    {
        if (ImGui::ImageButton(("##" + std::to_string(++internal_draw_id)).c_str(), ctx.add_image(asset->get_thumbnail()), {64, 64}, {0, 1}, {1, 0}))
        {
        }
    }
    else
    {
        if (ImGui::Button(("#" + std::string(asset->get_name()) + "##" + std::to_string(++internal_draw_id)).c_str(), {64, 64}))
        {
        }
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}