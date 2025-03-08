#include "pr.hpp"
#include <iostream>
#include <memory>

int main() {
    using namespace pr;

    Config config;
    config.ScreenWidth = 1920;
    config.ScreenHeight = 1080;
    config.SwapInterval = 1;
    Initialize(config);

    Camera3D camera;
    camera.origin = { 0, 4, 4 };
    camera.lookat = { 0, 0, 0 };
    
    SetDataDir(ExecutableDir());

    std::string err;
    std::shared_ptr<FScene> scene = ReadWavefrontObj(GetDataPath("teapot.obj"), err);

    double e = GetElapsedTime();

    while (pr::NextFrame() == false) {
        if (IsImGuiUsingMouse() == false) {
            UpdateCameraBlenderLike(&camera);
        }

        ClearBackground(0.1f, 0.1f, 0.1f, 1);

        BeginCamera(camera);

        PushGraphicState();

        DrawGrid(GridAxis::XZ, 1.0f, 10, { 128, 128, 128 });
        DrawXYZAxis(1.0f);

        static glm::vec3 V = { 1, 1, 0 };
		ManipulatePosition(camera, &V, 0.5f);

        DrawArrow({ 0,0,0 }, V, 0.01f, { 255,0,255 });
        DrawText(V, "v", 30);

        {
            glm::vec3 t0, t1;
            GetOrthonormalBasis(glm::normalize(V), &t0, &t1);
            DrawFreeGrid({ 0,0,0 }, t0, t1, 5, { 128,  128, 128 });
        }

        scene->visitPolyMesh([](std::shared_ptr<const FPolyMeshEntity> polymesh) {
            if (polymesh->visible() == false)
            {
                return;
            }
            ColumnView<int32_t> faceCounts(polymesh->faceCounts());
            ColumnView<int32_t> indices(polymesh->faceIndices());
            ColumnView<glm::vec3> positions(polymesh->positions());
            ColumnView<glm::vec3> normals(polymesh->normals());

            glm::mat3 vvt = glm::mat3(
                V.x * V.x, V.x * V.y, V.x * V.z,
                V.x * V.y, V.y * V.y, V.z * V.y,
                V.x * V.z, V.y * V.z, V.z * V.z
            );
            glm::mat3 householder = glm::identity<glm::mat3>() - vvt * (2.0f / glm::dot(V, V));

            glm::mat4 m = polymesh->localToWorld();
            m = glm::translate(m, { 1, 1, 1 });
            m = glm::scale(m, { 0.02f, 0.02f , 0.02f });

            auto sheet = polymesh->attributeSpreadsheet(pr::AttributeSpreadsheetType::Points);

            // Geometry
            for (auto transform : { std::make_pair( m, glm::u8vec3{128,128,128}), std::make_pair( glm::mat4(householder) * m, glm::u8vec3{255,255,255}) })
            {
                pr::SetObjectTransform(transform.first);

                pr::PrimBegin(pr::PrimitiveMode::Lines);
                for (int i = 0; i < positions.count(); i++)
                {
                    glm::vec3 p = positions[i];
                    pr::PrimVertex(p, transform.second);
                }
                int indexBase = 0;
                for (int i = 0; i < faceCounts.count(); i++)
                {
                    int nVerts = faceCounts[i];
                    for (int j = 0; j < nVerts; ++j)
                    {
                        int i0 = indices[indexBase + j];
                        int i1 = indices[indexBase + (j + 1) % nVerts];
                        pr::PrimIndex(i0);
                        pr::PrimIndex(i1);
                    }
                    indexBase += nVerts;
                }
                pr::PrimEnd();
                pr::SetObjectIdentify();
            }
        });

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
